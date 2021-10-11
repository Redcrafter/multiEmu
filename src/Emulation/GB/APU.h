#pragma once
#include <cassert>
#include <cstdint>

#include "../../audio.h"

namespace Gameboy {

const uint8_t WAVE_DUTY_TABLE[4][8] = {
	{ 0, 0, 0, 0, 0, 0, 0, 1 },
	{ 0, 0, 0, 0, 0, 0, 1, 1 },
	{ 0, 0, 0, 0, 1, 1, 1, 1 },
	{ 1, 1, 1, 1, 1, 1, 0, 0 }
};

struct Length {
	bool dacEnabled;

	bool channelEnabled;

	uint16_t lengthCounter;
	bool lengthEnabled;

	void clockLength() {
		if(lengthEnabled && lengthCounter > 0) {
			lengthCounter--;

			// The channel is disabled if the length counter is reset.
			if(lengthCounter == 0) {
				channelEnabled = false;
			}
		}
	}

	template<int val>
	void apuBug(uint8_t data, uint8_t fsStep) {
		// apu bug: Enabling in first half of length period should clock length
		if(data & 0x40 && !lengthEnabled && fsStep & 1 && lengthCounter > 0) {
			lengthCounter--;
			if(lengthCounter == 0) {
				if(data & 0x80) {
					lengthCounter = val;
				} else {
					channelEnabled = false;
				}
			}
		}

		lengthEnabled = (data >> 6) & 1;
	}
};

struct Envelope {
	uint8_t envelopePeriod;
	bool envelopeDir;
	uint8_t envelopeInitialVol;

	uint8_t envPeriodTimer;
	uint8_t envVolume;

	void writeEnvelope(uint8_t data) {
		envelopePeriod = data & 7;
		envelopeDir = (data & 8) != 0;
		envelopeInitialVol = data >> 4;
	}
	uint8_t readEnvelope() const {
		return envelopeInitialVol << 4 | envelopeDir << 3 | envelopePeriod;
	}

	void clockEnvelope() {
		if(envelopePeriod == 0) return;

		if(envPeriodTimer > 0) {
			envPeriodTimer--;
		}
		if(envPeriodTimer == 0) {
			envPeriodTimer = envelopePeriod;

			if((envVolume < 0xF && envelopeDir) || (envVolume > 0 && !envelopeDir)) {
				envVolume += envelopeDir ? 1 : -1;
			}
		}
	}
};

struct SquareBase : Envelope, Length {
	uint16_t frequency;

	uint8_t soundPattern;

	uint16_t freqTimer;
	uint8_t wavePos;

	void writeLength(uint8_t data) {
		lengthCounter = 64 - (data & 0x3F);
		soundPattern = data >> 6;
	}
	uint8_t readLength() const {
		return soundPattern << 6 | 0x3F;
	}

	void writeFreqLow(uint8_t data) {
		frequency = (frequency & ~0xFF) | data;
	}

	void clock() {
		if(freqTimer == 0) {
			freqTimer = (2048 - frequency) * 4;
			wavePos = (wavePos + 1) & 7;
		}

		freqTimer--;
	}

	float output() {
		if(dacEnabled && channelEnabled) {
			return (WAVE_DUTY_TABLE[soundPattern][wavePos] * envVolume) / 7.5 - 1;
		}
		return 0;
	}
};

struct Square1 : SquareBase {
	uint8_t sweepShift;
	bool sweepDir;
	uint8_t sweepPeriod;

	bool sweepEnable;
	uint8_t sweepTimer;
	uint16_t shadowFrequency;

	uint16_t calculateFrequency() {
		auto newFrequency = shadowFrequency >> sweepShift;
		newFrequency = shadowFrequency + (sweepDir ? -newFrequency : newFrequency);

		/* overflow check */
		if(newFrequency > 2047) {
			channelEnabled = false;
		}

		return newFrequency;
	}

	void clockSweep() {
		if(sweepTimer > 0) {
			sweepTimer--;
		}

		if(sweepTimer == 0) {
			sweepTimer = sweepPeriod > 0 ? sweepPeriod : 8;

			if(sweepEnable && sweepPeriod > 0) {
				auto newFrequency = calculateFrequency();

				if(newFrequency <= 2047 && sweepShift > 0) {
					frequency = newFrequency;
					shadowFrequency = newFrequency;

					/* for overflow check */
					calculateFrequency();
				}
			}
		}
	}
};
struct Square2 : SquareBase {};

struct Wave : Length {
	uint8_t outputLevel;
	uint16_t frequency;
	uint8_t waveRam[16];

	uint16_t freqTimer;
	uint8_t wavePos;

	void clock() {
		if(freqTimer == 0) {
			freqTimer = (2048 - frequency) * 2;
			wavePos = (wavePos + 1) & 31;
		}

		freqTimer--;
	}

	float output() {
		// TODO: not channelEnabled?
		if(dacEnabled && channelEnabled) {
			auto sample = (waveRam[wavePos / 2] >> (wavePos & 1 ? 4 : 0)) & 0xF;
			int volumeShift;
			switch(outputLevel) {
				case 0: volumeShift = 4; break;
				case 1: volumeShift = 0; break;
				case 2: volumeShift = 1; break;
				case 3: volumeShift = 2; break;
			}
			return (sample >> volumeShift) / 7.5 - 1;
		}
		return 0;
	}
};
struct Noise : Envelope, Length {
	uint8_t divider;
	bool counterStep;
	uint8_t shiftClock;

	uint16_t freqTimer;
	uint16_t lfsr;

	void writePoly(uint8_t data) {
		divider = data & 7;
		counterStep = data & 8;
		shiftClock = data >> 4;
	}

	void clock() {
		if(freqTimer == 0) {
			freqTimer = (divider == 0 ? 8 : divider << 4) << shiftClock;

			auto xorResult = (lfsr & 1) ^ ((lfsr & 2) >> 1);

			lfsr = (lfsr >> 1) | (xorResult << 14);

			if(counterStep) {
				lfsr &= !(1 << 6);
				lfsr |= xorResult << 6;
			}
		}
		freqTimer--;
	}

	float output() {
		if(dacEnabled && channelEnabled) {
			return ((!(lfsr & 1)) * envVolume) / 7.5 - 1;
		}
		return 0;
	}
};

class APU {
  private:
	bool enabled = true;

	Square1 ch1;
	Square2 ch2;
	Wave ch3;
	Noise ch4;

	uint16_t cycles = 0;
	uint16_t fsStep = 0;
	uint16_t sampleCounter = 0;

	uint8_t nr50;
	uint8_t nr51;

  public:
	uint8_t read(uint16_t address) const {
		// printf("%04X read\n", address);

		switch(address) {
			case 0xFF10: return 0x80 | (ch1.sweepPeriod << 4) | (ch1.sweepDir << 3) | (ch1.sweepShift);
			case 0xFF11: return ch1.readLength();
			case 0xFF12: return ch1.readEnvelope();
			case 0xFF13: return 0xFF;
			case 0xFF14: return ch1.lengthEnabled << 6 | 0xBF;

			case 0xFF16: return ch2.readLength();
			case 0xFF17: return ch2.readEnvelope();
			case 0xFF18: return 0xFF;
			case 0xFF19: return ch2.lengthEnabled << 6 | 0xBF;

			case 0xFF1A: return ch3.dacEnabled << 7 | 0x7F;
			case 0xFF1B: return 0xFF;
			case 0xFF1C: return ch3.outputLevel << 5 | 0x9F;
			case 0xFF1D: return 0xFF;
			case 0xFF1E: return ch3.lengthEnabled << 6 | 0xBF;

			case 0xFF20: return 0xFF;
			case 0xFF21: return ch4.readEnvelope();
			case 0xFF22: return ch4.shiftClock << 4 | ch4.counterStep << 3 | ch4.divider;
			case 0xFF23: return ch4.lengthEnabled << 6 | 0xBF;

			case 0xFF24: return nr50;
			case 0xFF25: return nr51;
			case 0xFF26:
				return ch1.channelEnabled |
					   ch2.channelEnabled << 1 |
					   ch3.channelEnabled << 2 |
					   ch4.channelEnabled << 3 |
					   0x70 |
					   enabled << 7;
			case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37:
			case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
				return ch3.waveRam[address - 0xFF30];
		}

		return 0;
	}

	void write(uint16_t address, uint8_t data) {
		// printf("%04X write %02X\n", address, data);

		if(!enabled && address != 0xFF26)
			return;

		switch(address) {
#pragma region Sound Channel 1
			case 0xFF10: // Sweep register
				ch1.sweepShift = data & 7;
				ch1.sweepDir = data & 8;
				ch1.sweepPeriod = (data >> 4) & 7;
				break;
			case 0xFF11: ch1.writeLength(data); break;
			case 0xFF12:
				ch1.writeEnvelope(data);
				ch1.dacEnabled = (data & 0xF8) != 0;
				if(!ch1.dacEnabled) ch1.channelEnabled = false;
				break;
			case 0xFF13: ch1.writeFreqLow(data); break;
			case 0xFF14:
				ch1.frequency = (ch1.frequency & 0xFF) | ((data & 7) << 8);

				if(data & 0x80 && ch1.dacEnabled) {
					ch1.channelEnabled = true;

					ch1.envPeriodTimer = ch1.sweepPeriod;
					ch1.envVolume = ch1.envelopeInitialVol;

					ch1.shadowFrequency = ch1.frequency;

					ch1.sweepTimer = ch1.sweepPeriod > 0 ? ch1.sweepPeriod : 8;
					ch1.sweepEnable = ch1.sweepPeriod > 0 || ch1.sweepShift > 0;
					if(ch1.sweepShift > 0) ch1.calculateFrequency();

					if(ch1.lengthCounter == 0) {
						ch1.lengthCounter = 64;
						ch1.lengthEnabled = false;
					}
				}

				ch1.apuBug<63>(data, fsStep);
				break;
#pragma endregion
#pragma region Sound Channel 2:
			case 0xFF16: ch2.writeLength(data); break;
			case 0xFF17:
				ch2.writeEnvelope(data);
				ch2.dacEnabled = (data & 0xF8) != 0;
				if(!ch2.dacEnabled) ch2.channelEnabled = false;
				break;
			case 0xFF18: ch2.writeFreqLow(data); break;
			case 0xFF19:
				// ch2.writeFreqHigh(data);
				ch2.frequency = (ch2.frequency & 0xFF) | ((data & 7) << 8);

				if(data & 0x80 && ch2.dacEnabled) {
					ch2.channelEnabled = true;

					// Envelope is triggered.
					ch2.envPeriodTimer = ch2.envelopePeriod;
					ch2.envVolume = ch2.envelopeInitialVol;

					if(ch2.lengthCounter == 0) {
						ch2.lengthCounter = 64;
						ch2.lengthEnabled = false;
					}
				}

				ch2.apuBug<63>(data, fsStep);
				break;
#pragma endregion
#pragma region Sound Channel 3:
			case 0xFF1A:
				ch3.dacEnabled = data >> 7;
				if(!ch3.dacEnabled) ch3.channelEnabled = false;
				break;
			case 0xFF1B: ch3.lengthCounter = 256 - data; break;
			case 0xFF1C: ch3.outputLevel = (data >> 5) & 3; break;
			case 0xFF1D: ch3.frequency = (ch3.frequency & ~0xFF) | data; break;
			case 0xFF1E:
				ch3.frequency = (ch3.frequency & 0xFF) | ((data & 7) << 8);

				if(data & 0x80) {
					if(ch3.dacEnabled) ch3.channelEnabled = true;

					ch3.wavePos = 0;
					if(ch3.lengthCounter == 0) {
						ch3.lengthCounter = 256;
						ch3.lengthEnabled = false;
					}
				}

				ch3.apuBug<255>(data, fsStep);
				break;
			// FF30-FF3F Wave Pattern RAM
			case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37:
			case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F: 
				// TODO weird behaviour while playing>
				ch3.waveRam[address - 0xFF30] = data;
				break;
#pragma endregion
#pragma region Sound Channel 4:
			case 0xFF20: ch4.lengthCounter = 64 - (data & 0x3F); break;
			case 0xFF21:
				ch4.writeEnvelope(data);
				ch4.dacEnabled = (data & 0xF8) != 0;
				if(!ch4.dacEnabled) ch4.channelEnabled = false;
				break;
			case 0xFF22: ch4.writePoly(data); break;
			case 0xFF23:
				if(data & 0x80) {
					if(ch4.dacEnabled) ch4.channelEnabled = true;

					if(ch4.lengthCounter == 0) {
						ch4.lengthCounter = 64;
					}

					ch4.lfsr = 0x7FFF;
					ch4.envPeriodTimer = ch4.envelopePeriod;
					ch4.envVolume = ch4.envelopeInitialVol;
				}

				ch4.apuBug<63>(data, fsStep);

				ch4.lengthEnabled = (data >> 6) & 1;
				break;
#pragma endregion
			// Sound Control:
			case 0xFF24: nr50 = data; break;
			case 0xFF25: nr51 = data; break;
			case 0xFF26:
				if(!(data & 0x80) && enabled) {
					for(size_t i = 0xFF10; i < 0xFF26; i++) write(i, 0);
				} else if(data & 0x80 && !enabled) {
					fsStep = 0;
					ch1.wavePos = 0;
					ch2.wavePos = 0;
					ch3.wavePos = 0;
				}

				enabled = (data & 0x80) != 0;
				break;
		}
	}

	void clock() {
		for(size_t i = 0; i < 4; i++) {
			ch1.clock();
			ch2.clock();
			ch3.clock();
			ch4.clock();
		}

		// should be 0x1FFF but gets ticked 4 times
		if(cycles == 0x7FF) {
			switch(fsStep) {
				case 0:
				case 4:
					ch1.clockLength();
					ch2.clockLength();
					ch3.clockLength();
					ch4.clockLength();
					break;
				case 2:
				case 6:
					ch1.clockLength();
					ch2.clockLength();
					ch3.clockLength();
					ch4.clockLength();
					ch1.clockSweep();
					break;
				case 7:
					ch1.clockEnvelope();
					ch2.clockEnvelope();
					ch4.clockEnvelope();
					break;
			}

			fsStep = (fsStep + 1) & 7;
		}

		cycles = (cycles + 1) & 0x7FF;

		sampleCounter++;
		if(sampleCounter == 23) {
			sampleCounter = 0;

			auto leftVolume = (nr50 >> 4) & 7;
			// auto rightVolume = (nr50 >> 4) & 7;

			auto left = 0.0;
			if(nr51 & 0x10) left += ch1.output();
			if(nr51 & 0x20) left += ch2.output();
			if(nr51 & 0x40) left += ch3.output();
			if(nr51 & 0x80) left += ch4.output();
			left = (leftVolume / 7.0) * (left / 4.0);

			Audio::PushSample(left);
		}
	}

	void reset() {
		enabled = true;
		cycles = 0;

		write(0xFF10, 0x80);
		write(0xFF11, 0xBF);
		write(0xFF12, 0xF3);
		write(0xFF13, 0xFF);
		write(0xFF14, 0xBF);

		write(0xFF16, 0x3F);
		write(0xFF17, 0x00);
		write(0xFF18, 0xFF);
		write(0xFF19, 0xBF);

		write(0xFF1A, 0x7F);
		write(0xFF1B, 0xFF);
		write(0xFF1C, 0x9F);
		write(0xFF1D, 0xFF);
		write(0xFF1E, 0xBF);

		write(0xFF20, 0xFF);
		write(0xFF21, 0x00);
		write(0xFF22, 0x00);
		write(0xFF23, 0xBF);

		write(0xFF24, 0x77);
		write(0xFF25, 0xF3);
		write(0xFF26, 0xF1);

		for(size_t i = 0; i < 16; i++) {
			ch3.waveRam[i] = 0;
		}

		assert(read(0xFF10) == 0x80);
		assert(read(0xFF11) == 0xBF);
		assert(read(0xFF12) == 0xF3);
		assert(read(0xFF13) == 0xFF);
		assert(read(0xFF14) == 0xBF);

		assert(read(0xFF16) == 0x3F);
		assert(read(0xFF17) == 0x00);
		assert(read(0xFF18) == 0xFF);
		assert(read(0xFF19) == 0xBF);

		assert(read(0xFF1A) == 0x7F);
		assert(read(0xFF1B) == 0xFF);
		assert(read(0xFF1C) == 0x9F);
		assert(read(0xFF1D) == 0xFF);
		assert(read(0xFF1E) == 0xBF);

		assert(read(0xFF20) == 0xFF);
		assert(read(0xFF21) == 0x00);
		assert(read(0xFF22) == 0x00);
		assert(read(0xFF23) == 0xBF);

		assert(read(0xFF24) == 0x77);
		assert(read(0xFF25) == 0xF3);
		assert(read(0xFF26) == 0xF1);
	}
};

} // namespace Gameboy
