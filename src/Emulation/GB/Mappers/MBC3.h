#pragma once
#include "MBC.h"
#include <chrono>

namespace Gameboy {

typedef std::chrono::duration<int32_t, std::ratio<24 * 60 * 60>> days;

class MBC3 final : public MBC {
	// will increase when paused
	std::chrono::system_clock::time_point startTime;
	std::chrono::system_clock::duration passedTime { 0 };

	uint32_t romBank1 = 0x4000;

	uint8_t rtcRegs[5];

	uint8_t aSelect = 0;
	bool lastTimer = true;

	bool ramEnable = false;
	bool hasTimer;

  public:
	MBC3(const std::vector<uint8_t>& rom, uint32_t ramSize, bool hasBattery, bool hasTimer) : MBC(rom, ramSize, hasBattery), hasTimer(hasTimer) {
		startTime = std::chrono::system_clock::now();
	}

	~MBC3() override = default;

	uint8_t Read0(uint16_t addr) const override { return rom[addr & 0x3FFF]; }
	uint8_t Read4(uint16_t addr) const override { return rom[romBank1 | (addr & 0x3FFF)]; };
	uint8_t ReadA(uint16_t addr) const override {
		if(aSelect < 4 && ramEnable) {
			return ram[((aSelect << 13) | (addr & 0x1FFF)) & ramMask];
		}

		if(hasTimer && aSelect >= 8 && aSelect < 0xC) {
			return rtcRegs[aSelect - 8];
		}
		if(hasTimer && aSelect == 0xC) {
			// TODO
			return rtcRegs[4];
		}

		return 0xFF;
	}

	void Write0(uint16_t addr, uint8_t val) override {
		if(addr < 0x2000) {
			// 0000-1FFF - RAM Enable
			ramEnable = !ram.empty() && (val & 0xF) == 0xA;
		} else {
			// 2000-3FFF - ROM Bank Number
			romBank1 = (0x4000 * std::max(val & 0x8F, 1)) & romMask;
		}
	}
	void Write4(uint16_t addr, uint8_t val) override {
		if(addr < 0x6000) {
			// 4000-5FFF - RAM Bank Number - or - RTC Register Select
			aSelect = val & 0xF;
		} else {
			// 6000-7FFF - Latch Clock Data
			
			if(!lastTimer && val) {
				if(!(rtcRegs[4] & 0x40)) {
					const auto now = std::chrono::system_clock::now();
					passedTime += now - startTime;
					startTime = now;
				}
				auto seconds = std::chrono::duration_cast<std::chrono::seconds>(passedTime);

				for(int i = 0; i < seconds.count(); ++i) {
					// seconds
					rtcRegs[0] = (rtcRegs[0] + 1) & 0x3F;

					if(rtcRegs[0] == 60) {
						// minutes
						rtcRegs[0] = 0;
						rtcRegs[1] = (rtcRegs[1] + 1) & 0x3F;

						if(rtcRegs[1] == 60) {
							// hours
							rtcRegs[1] = 0;
							rtcRegs[2] = (rtcRegs[2] + 1) & 0x1F;

							if(rtcRegs[2] == 24) {
								// days
								rtcRegs[2] = 0;
								rtcRegs[3]++;

								if(rtcRegs[3] == 0) {
									rtcRegs[4] ^= 1;

									if(!(rtcRegs[4] & 1)) {
										rtcRegs[4] |= 0x80;
									}
								}
							}
						}
					}
				}
				
				passedTime -= seconds;
			}
			lastTimer = val != 0;
		}
	}
	void WriteA(uint16_t addr, uint8_t val) override {
		if(aSelect < 4 && ramEnable) {
			ram[((aSelect << 13) | (addr & 0x1FFF)) & ramMask] = val;
			return;
		}

		if(hasTimer) {
			switch(aSelect) {
				case 0x8: rtcRegs[0] = val & 0x3F; break;
				case 0x9: rtcRegs[1] = val & 0x3F; break;
				case 0xA: rtcRegs[2] = val & 0x1F; break;
				case 0xB: rtcRegs[3] = val & 0xFF; break;
				case 0xC:
					const bool newHalt = val & 0x40;
					if(!(rtcRegs[4] & 0x40) && newHalt) {
						// stop timer
						passedTime += std::chrono::system_clock::now() - startTime;
					} else if((rtcRegs[4] & 0x40) && !newHalt) {
						// start timer
						startTime = std::chrono::system_clock::now();
					}
					rtcRegs[4] = val & 0xC1;
			}
		}
	}

	void SaveState(saver& saver) override {
		saver.write(ram.data(), ram.size());
		saver << romBank1;
		saver << aSelect;
		saver << ramEnable;
	}
	void LoadState(saver& saver) override {
		saver.read(ram.data(), ram.size());
		saver >> romBank1;
		saver >> aSelect;
		saver >> ramEnable;
	}
};

}
