#pragma once
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
	uint16_t lengthCounter;

	bool dacEnabled;
	bool channelEnabled;
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

	void SaveLength(saver& saver) {
		saver << lengthCounter;
		saver << dacEnabled;
		saver << channelEnabled;
		saver << lengthEnabled;
	}
	void LoadLength(saver& saver) {
		saver >> lengthCounter;
		saver >> dacEnabled;
		saver >> channelEnabled;
		saver >> lengthEnabled;
	}
};

struct Envelope {
	bool envelopeDir;
	uint8_t envelopePeriod;
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

	void SaveEnvelope(saver& saver) {
		saver << envelopeDir;
		saver << envelopePeriod;
		saver << envelopeInitialVol;
		saver << envPeriodTimer;
		saver << envVolume;
	}
	void LoadEnvelope(saver& saver) {
		saver >> envelopeDir;
		saver >> envelopePeriod;
		saver >> envelopeInitialVol;
		saver >> envPeriodTimer;
		saver >> envVolume;
	}
};

struct SquareBase : Envelope, Length {
	uint16_t frequency;
	uint16_t freqTimer;
	uint8_t soundPattern;
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
			freqTimer = 2048 - frequency;
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

	void SaveSquare(saver& saver) {
		SaveEnvelope(saver);
		SaveLength(saver);
		saver << frequency;
		saver << freqTimer;
		saver << soundPattern;
		saver << wavePos;
	}
	void LoadSquare(saver& saver) {
		LoadEnvelope(saver);
		LoadLength(saver);
		saver >> frequency;
		saver >> freqTimer;
		saver >> soundPattern;
		saver >> wavePos;
	}
};

struct Square1 : SquareBase {
	uint16_t shadowFrequency;

	uint8_t sweepShift;
	uint8_t sweepPeriod;
	uint8_t sweepTimer;

	bool sweepDir;
	bool sweepEnable;

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

				if(newFrequency < 2048 && sweepShift > 0) {
					frequency = newFrequency;
					shadowFrequency = newFrequency;

					/* for overflow check */
					calculateFrequency();
				}
			}
		}
	}

	void Save(saver& saver) {
		SaveSquare(saver);
		saver << shadowFrequency;
		saver << sweepShift;
		saver << sweepPeriod;
		saver << sweepTimer;
		saver << sweepDir;
		saver << sweepEnable;
	}
	void Load(saver& saver) {
		LoadSquare(saver);
		saver >> shadowFrequency;
		saver >> sweepShift;
		saver >> sweepPeriod;
		saver >> sweepTimer;
		saver >> sweepDir;
		saver >> sweepEnable;
	}
};
struct Square2 : SquareBase {};

struct Wave : Length {
	uint8_t waveRam[16];
	uint16_t frequency;
	uint16_t freqTimer;
	uint8_t outputLevel;
	uint8_t wavePos;

	void clock() {
		if(freqTimer == 0) {
			freqTimer = 2048 - frequency;
			wavePos = (wavePos + 1) & 31;
		}
		freqTimer--;

		if(freqTimer == 0) {
			freqTimer = 2048 - frequency;
			wavePos = (wavePos + 1) & 31;
		}
		freqTimer--;
	}

	float output() {
		// TODO: not channelEnabled?
		if(dacEnabled && channelEnabled) {
			auto sample = (waveRam[wavePos / 2] >> (wavePos & 1 ? 4 : 0)) & 0xF;
			int volumeShift = 0;
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

	void Save(saver& saver) {
		SaveLength(saver);
		saver << waveRam;
		saver << frequency;
		saver << freqTimer;
		saver << outputLevel;
		saver << wavePos;
	}
	void Load(saver& saver) {
		LoadLength(saver);
		saver >> waveRam;
		saver >> frequency;
		saver >> freqTimer;
		saver >> outputLevel;
		saver >> wavePos;
	}
};
struct Noise : Envelope, Length {
	uint16_t freqTimer;
	uint16_t lfsr;
	uint8_t divider;
	uint8_t shiftClock;
	bool counterStep;

	void writePoly(uint8_t data) {
		divider = data & 7;
		counterStep = data & 8;
		shiftClock = data >> 4;
	}

	void clock() {
		if(freqTimer == 0) {
			freqTimer = (divider == 0 ? 2 : divider << 2) << shiftClock;

			auto mask = counterStep ? 0x4040 : 0x4000;
			auto newHigh = (lfsr ^ (lfsr >> 1)) & 1;
			lfsr >>= 1;
			lfsr = newHigh ? lfsr | mask : lfsr & ~mask;
		}
		freqTimer--;
	}

	float output() {
		if(dacEnabled && channelEnabled) {
			return ((!(lfsr & 1)) * envVolume) / 7.5f - 1;
		}
		return 0;
	}

	void Save(saver& saver) {
		SaveEnvelope(saver);
		SaveLength(saver);
		saver << freqTimer;
		saver << lfsr;
		saver << divider;
		saver << shiftClock;
		saver << counterStep;
	}
	void Load(saver& saver) {
		LoadEnvelope(saver);
		LoadLength(saver);
		saver >> freqTimer;
		saver >> lfsr;
		saver >> divider;
		saver >> shiftClock;
		saver >> counterStep;
	}
};

class APU {
  private:
	Square1 ch1;
	Square2 ch2;
	Wave ch3;
	Noise ch4;

	uint16_t cycles;
	uint16_t fsStep;
	uint16_t sampleCounter;
	uint8_t nr50;
	uint8_t nr51;
	bool enabled;

  public:
	bool gbc;

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
				return ch3.channelEnabled ? ch3.waveRam[ch3.wavePos / 2] : ch3.waveRam[address - 0xFF30];
		}

		return 0;
	}

	uint8_t FF76() const {
		if(!gbc) return 0xFF;

		uint8_t val = 0;
		if(ch1.dacEnabled && ch1.channelEnabled) {
			val |= (WAVE_DUTY_TABLE[ch1.soundPattern][ch1.wavePos] * ch1.envVolume) & 0xF;
		}
		if(ch2.dacEnabled && ch2.channelEnabled) {
			val |= (WAVE_DUTY_TABLE[ch2.soundPattern][ch2.wavePos] * ch2.envVolume) << 4;
		}
		return val;
	}
	uint8_t FF77() const {
		return 0xFF;
	}

	void write(uint16_t address, uint8_t data) {
		// printf("%04X write %02X\n", address, data);

		if(!enabled && address != 0xFF26)
			return;

		switch(address) {
#pragma region Sound Channel 1
			case 0xFF10: // Sweep register
				ch1.sweepShift = data & 7;
				ch1.sweepDir = (data & 8) != 0;
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

					ch1.envPeriodTimer = ch1.envelopePeriod;
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
						ch4.lengthEnabled = false;
					}

					ch4.lfsr = 0x7FFF;
					ch4.envPeriodTimer = ch4.envelopePeriod;
					ch4.envVolume = ch4.envelopeInitialVol;
				}

				ch4.apuBug<63>(data, fsStep);
				break;
#pragma endregion
			// Sound Control:
			case 0xFF24: nr50 = data; break;
			case 0xFF25: nr51 = data; break;
			case 0xFF26: // NR52
				if(!(data & 0x80) && enabled) {
					fsStep = 0;
					ch1.wavePos = 0;
					ch2.wavePos = 0;
					ch3.wavePos = 0;
					
					for(size_t i = 0xFF10; i < 0xFF26; i++) write(i, 0);
				} else if(data & 0x80 && !enabled) {
				}

				enabled = (data & 0x80) != 0;
				break;
		}
	}

	void clock() {
		if(!enabled) return;

		ch1.clock();
		ch2.clock();
		ch3.clock();
		ch4.clock();

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

			auto left = 0.0;
			auto right = 0.0;

			// todo: cache output values?

			if(nr51 & 0x10) left += ch1.output();
			if(nr51 & 0x01) right += ch1.output();

			if(nr51 & 0x20) left += ch2.output();
			if(nr51 & 0x02) right += ch2.output();

			if(nr51 & 0x40) left += ch3.output();
			if(nr51 & 0x04) right += ch3.output();

			if(nr51 & 0x80) left += ch4.output();
			if(nr51 & 0x08) right += ch4.output();

			auto leftVolume = (nr50 >> 4) & 7;
			auto rightVolume = (nr50 >> 4) & 7;

			left = (leftVolume / 7.0) * (left / 4.0);
			right = (rightVolume / 7.0) * (right / 4.0);

			Audio::PushSample(left, right);
		}
	}

	void reset() {
		enabled = true;
		cycles = 0;
		fsStep = 0;
		sampleCounter = 0;

		// write(0xFF10, 0x80);
		ch1.sweepShift = 0;
		ch1.sweepDir = 0;
		ch1.sweepPeriod = 0;

		// write(0xFF11, 0xBF);
		ch1.lengthCounter = 1;
		ch1.soundPattern = 2;

		// write(0xFF12, 0xF3);
		ch1.envelopePeriod = 3;
		ch1.envelopeDir = false;
		ch1.envelopeInitialVol = 0xF;

		// write(0xFF13, 0xFF);
		ch1.frequency = 0;
		// write(0xFF14, 0xBF);
		ch1.lengthEnabled = false;

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
		ch1.channelEnabled = true;

		for(size_t i = 0; i < 16; i++) {
			ch3.waveRam[i] = 0;
		}
	}

	void SaveState(saver& saver) {
		ch1.Save(saver);
		ch2.SaveSquare(saver);
		ch3.Save(saver);
		ch4.Save(saver);

		saver << cycles;
		saver << fsStep;
		saver << sampleCounter;
		saver << nr50;
		saver << nr51;
		saver << enabled;
	}
	void LoadState(saver& saver) {
		ch1.Load(saver);
		ch2.LoadSquare(saver);
		ch3.Load(saver);
		ch4.Load(saver);

		saver >> cycles;
		saver >> fsStep;
		saver >> sampleCounter;
		saver >> nr50;
		saver >> nr51;
		saver >> enabled;
	}
};

} // namespace Gameboy
