#include "RP2A03.h"

#include "../../audio.h"
#include "Bus.h"

#include <cassert>

namespace Nes {

// clang-format off
const uint8_t lengthTable[] = {
	10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
	12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};
const uint8_t dutyTable[4][8] = {
	{0, 1, 0, 0, 0, 0, 0, 0},
	{0, 1, 1, 0, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0},
	{1, 0, 0, 1, 1, 1, 1, 1}
};

const uint8_t triangleTable[] = {
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
	0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
const uint16_t noiseTable[] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
const uint16_t dmcTable[]   = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };
// clang-format on

void Pulse::Clock() {
	if(timer == 0) {
		timer = timerPeriod;

		dutyValue = (dutyValue + 1) % 8;
	} else {
		timer--;
	}
}

void vrc6Pulse::Clock(uint8_t freqShift) {
	if(!enabled) {
		return;
	}
	if(timer == 0) {
		timer = timerPeriod >> freqShift;

		if(dutyValue == 0) {
			dutyValue = 15;
		} else {
			dutyValue--;
		}
	} else {
		timer--;
	}
}

uint8_t vrc6Pulse::Output() {
	return enabled && (mode || dutyValue <= dutyCycle) ? volume : 0;
}

void vrc6Sawtooth::Clock(uint8_t freqShift) {
	if(!enabled) {
		return;
	}
	if(timer == 0) {
		timer = timerPeriod >> freqShift;

		if(step != 0 && step % 2 == 0) {
			accumulator += accumRate;
		}

		step++;
		if(step == 14) {
			step = 0;
			accumulator = 0;
		}
	} else {
		timer--;
	}
}

uint8_t vrc6Sawtooth::Output() {
	return enabled ? accumulator >> 3 : 0;
}

void Triangle::Clock() {
	if(timerPeriod < 2) {
		return; // High pass filter cheat
	}
	if(timer == 0) {
		timer = timerPeriod;

		if(linearCounter > 0 && lengthCounter > 0) {
			dutyValue = (dutyValue + 1) & 0x1F;
		}
	} else {
		timer--;
	}
}

void Triangle::SaveState(nlohmann::json& saver) const {
	SaveBase(saver);

	saver["dutyValue"] = dutyValue;
	saver["linearCounterPeriod"] = linearCounterPeriod;
	saver["linearCounter"] = linearCounter;
	saver["linearCounterReload"] = linearCounterReload;
}

void Triangle::LoadState(const nlohmann::json& saver) {
	LoadBase(saver);

	dutyValue = saver["dutyValue"];
	linearCounterPeriod = saver["linearCounterPeriod"];
	linearCounter = saver["linearCounter"];
	linearCounterReload = saver["linearCounterReload"];
}

void Noise::Clock() {
	if(timer == 0) {
		timer = timerPeriod;

		uint8_t shift;
		if(mode) {
			shift = 6;
		} else {
			shift = 1;
		}

		auto b1 = shiftRegister & 1;
		auto b2 = (shiftRegister >> shift) & 1;

		shiftRegister >>= 1;
		shiftRegister |= (b1 ^ b2) << 14;
	} else {
		timer--;
	}
}

void SoundBase::ClockLength() {
	if(!lengthCounterEnabled && lengthCounter > 0) {
		lengthCounter--;
	}
}

void Envelope::ClockEnvelope() {
	if(envelopeStart) {
		envelopeVolume = 15;
		envelopeValue = envelopePeriod;
		envelopeStart = false;
	} else {
		if(envelopeValue > 0) {
			envelopeValue--;
		} else {
			envelopeValue = envelopePeriod;
			if(envelopeVolume > 0) {
				envelopeVolume--;
			} else {
				if(envelopeLoop)
					envelopeVolume = 15;
			}
		}
	}
}

void Pulse::WriteControl(uint8_t data) {
	dutyCycle = (data >> 6) & 3;

	lengthCounterEnabled = ((data >> 5) & 1);
	envelopeLoop = (data >> 5) & 1;

	envelopeEnabled = ((data >> 4) & 1);
	envelopePeriod = data & 0xF;
	constantVolume = data & 0xF;
	envelopeStart = true;
}

void Pulse::WriteSweep(uint8_t data) {
	sweepEnabled = (data >> 7) & 1;

	sweepPeriod = ((data >> 4) & 7);
	sweepNegate = (data >> 3) & 1;
	sweepShift = data & 7;
	sweepReload = true;
}

void Pulse::WriteTimerLow(uint8_t data) {
	timerPeriod = (timerPeriod & 0xFF00) | data;
}

void Pulse::WriteTimerHigh(uint8_t data) {
	if(enabled) {
		lengthCounter = lengthTable[data >> 3];
	}

	timerPeriod = (timerPeriod & 0x00FF) | ((data & 7) << 8);
	envelopeStart = true;
	dutyValue = 0;
}

void Pulse::ClockSweep() {
	if(sweepValue == 0 && sweepEnabled /*&& !(!sweepNegate && timerPeriod + (timerPeriod >> sweepShift) > 0x7FF)*/) {
		if(sweepNegate) {
			timerPeriod -= (timerPeriod >> sweepShift) + negative;
		} else {
			timerPeriod += timerPeriod >> sweepShift;
		}
	}
	if(sweepValue == 0 || sweepReload) {
		sweepValue = sweepPeriod;
		sweepReload = false;
	} else {
		sweepValue--;
	}
}

uint8_t Pulse::Output() const {
	if(!enabled) {
		return 0;
	}
	if(lengthCounter == 0) {
		return 0;
	}
	if(dutyTable[dutyCycle][dutyValue] == 0) {
		return 0;
	}

	if(timerPeriod < 8 || timerPeriod > 0x7FF) {
		return 0;
	}

	if(!sweepNegate && timerPeriod + (timerPeriod >> sweepShift) > 0x7FF) {
		return 0;
	}

	// if(timer < 8) {
	//	return 0;
	// }

	if(envelopeEnabled) {
		return constantVolume;
	} else {
		return envelopeVolume;
	}
}

void Pulse::SaveState(nlohmann::json& saver) const {
	SaveBase(saver);
	SaveEnv(saver);

	saver["negative"] = negative;
	saver["dutyCycle"] = dutyCycle;
	saver["dutyValue"] = dutyValue;
	saver["sweepReload"] = sweepReload;
	saver["sweepEnabled"] = sweepEnabled;
	saver["sweepNegate"] = sweepNegate;
	saver["sweepShift"] = sweepShift;
	saver["sweepPeriod"] = sweepPeriod;
	saver["sweepValue"] = sweepValue;
}

void Pulse::LoadState(const nlohmann::json& saver) {
	LoadBase(saver);
	LoadEnv(saver);

	negative = saver["negative"];
	dutyCycle = saver["dutyCycle"];
	dutyValue = saver["dutyValue"];
	sweepReload = saver["sweepReload"];
	sweepEnabled = saver["sweepEnabled"];
	sweepNegate = saver["sweepNegate"];
	sweepShift = saver["sweepShift"];
	sweepPeriod = saver["sweepPeriod"];
	sweepValue = saver["sweepValue"];
}

void DMC::Clock(Bus& bus) {
	if(timer > 0) {
		timer -= 2;
		return;
	}
	timer = timerPeriod;

	if(!silence) {
		int step = (shiftRegister & 1) * 4 - 2;

		if(unsigned(value + step) <= 0x7F) {
			value += step;
		}
	}

	shiftRegister >>= 1;
	bitCount--;

	if(bitCount == 0) {
		bitCount = 8;

		if(bufferEmpty) {
			silence = true;
		} else {
			silence = false;
			shiftRegister = sampleBuffer;
			bufferEmpty = true;

			FillBuffer(bus);
		}
	}
}

void DMC::Reload() {
	currentAddress = sampleAddress;
	currentLength = sampleLength;
}

void DMC::FillBuffer(Bus& bus) {
	if(bufferEmpty && currentLength > 0) {
		// not perfectly accurate
		bus.CpuStall = 2; // stalls cpu 1-4 cycles todo: https://www.nesdev.org/wiki/DMA
		sampleBuffer = bus.CpuRead(currentAddress);
		if(currentAddress == 0xFFFF) {
			currentAddress = 0x8000;
		} else {
			currentAddress++;
		}
		bufferEmpty = false;

		currentLength--;
		if(currentLength == 0) {
			if(loop) {
				Reload();
			} else {
				irq = irqEnable;
			}
		}
	}
}

void DMC::SaveState(nlohmann::json& saver) const {
	saver["enabled"] = enabled;
	saver["irq"] = irq;
	saver["irqEnable"] = irqEnable;
	saver["loop"] = loop;
	saver["silence"] = silence;
	saver["value"] = value;
	saver["sampleAddress"] = sampleAddress;
	saver["sampleLength"] = sampleLength;
	saver["currentAddress"] = currentAddress;
	saver["currentLength"] = currentLength;
	saver["bufferEmpty"] = bufferEmpty;
	saver["sampleBuffer"] = sampleBuffer;
	saver["shiftRegister"] = shiftRegister;
	saver["bitCount"] = bitCount;
	saver["timer"] = timer;
	saver["timerPeriod"] = timerPeriod;
}

void DMC::LoadState(const nlohmann::json& saver) {
	enabled = saver["enabled"];
	irq = saver["irq"];
	irqEnable = saver["irqEnable"];
	loop = saver["loop"];
	silence = saver["silence"];
	value = saver["value"];
	sampleAddress = saver["sampleAddress"];
	sampleLength = saver["sampleLength"];
	currentAddress = saver["currentAddress"];
	currentLength = saver["currentLength"];
	bufferEmpty = saver["bufferEmpty"];
	sampleBuffer = saver["sampleBuffer"];
	shiftRegister = saver["shiftRegister"];
	bitCount = saver["bitCount"];
	timer = saver["timer"];
	timerPeriod = saver["timerPeriod"];
}

RP2A03::RP2A03(Bus* bus) : bus(bus) {
	pulse1.negative = 1;
}

void RP2A03::Clock() {
	if(frameCounterMode) {
		switch(frameCounter) {
			case 7457:
			case 22371:
				ClockEnvelope();
				break;
			case 14913:
			case 37281:
				ClockEnvelope();
				ClockLength();
				break;
		}
	} else {
		switch(frameCounter) {
			case 7457:
			case 22371:
				ClockEnvelope();
				break;
			case 14913:
				ClockEnvelope();
				ClockLength();
				break;
			case 29829:
				ClockEnvelope();
				ClockLength();
				[[fallthrough]];
			case 29828:
			case 29830:
				if(!IRQinhibit) {
					Irq = true;
				}
				break;
		}
	}

	if(frameCounter % 40 == 0)
		GenerateSample();

	if(frameCounter % 2 == 0) {
		pulse1.Clock();
		pulse2.Clock();
		noise.Clock();
		dmc.Clock(*bus);
	}
	triangle.Clock();
	if(vrc6 && !vrc6Halt) {
		vrc6Pulse1.Clock(vrc6FreqShift);
		vrc6Pulse2.Clock(vrc6FreqShift);
		vrc6Saw.Clock(vrc6FreqShift);
	}

	frameCounter++;
	if(frameCounterMode) {
		if(frameCounter == 37282) {
			frameCounter = 0;
		}
	} else {
		if(frameCounter == 29831) {
			frameCounter = 1;
		}
	}
}

void RP2A03::Reset() {
	pulse1.enabled = false;
	pulse2.enabled = false;
	triangle.enabled = false;
	noise.enabled = false;
	dmc.enabled = false;

	dmc.timerPeriod = dmcTable[0];
	dmc.value = 0;
	dmc.irq = false;
	dmc.irqEnable = false;

	CpuWrite(0x4017, last4017Write);
	frameCounter += 10;
	Irq = false;

	pulse1.lengthCounterEnabled = true;
	pulse2.lengthCounterEnabled = true;
	noise.lengthCounterEnabled = true;
}

void RP2A03::HardReset() {
	/*
	Irq = true;
	last4017Write = 0;
	frameCounterMode = false;
	frameCounter = 0;
	IRQinhibit = false;

	std::memset(&pulse1, 0, sizeof(Pulse));
	std::memset(&pulse2, 0, sizeof(Pulse));
	std::memset(&triangle, 0, sizeof(Triangle));
	std::memset(&noise, 0, sizeof(Noise));
	std::memset(&dmc, 0, sizeof(DMC));

	vrc6 = false;
	vrc6Halt = false;
	vrc6FreqShift = 0;
	std::memset(&vrc6Pulse1, 0, sizeof(vrc6Pulse));
	std::memset(&vrc6Pulse2, 0, sizeof(vrc6Pulse));
	std::memset(&vrc6Saw, 0, sizeof(vrc6Sawtooth));

	bufferPos = 0;
	lastBufferPos = 0;
	std::memset(&waveBuffer, 0, sizeof(waveBuffer));
	*/

	// std::memset(&dmc, 0, sizeof(DMC));
	dmc.enabled = false;
	dmc.irq = false;
	dmc.irqEnable = false;
	dmc.loop = false;

	dmc.silence = true;

	dmc.value = 0;
	dmc.sampleAddress = 0;
	dmc.sampleLength = 0;

	dmc.bufferEmpty = true;

	dmc.timerPeriod = 0;
}

void RP2A03::CpuWrite(uint16_t addr, uint8_t data) {
	switch(addr) {
		#pragma region Pulse
		case 0x4000: pulse1.WriteControl(data); return;
		case 0x4001: pulse1.WriteSweep(data); return;
		case 0x4002: pulse1.WriteTimerLow(data); return;
		case 0x4003: pulse1.WriteTimerHigh(data); return;
		case 0x4004: pulse2.WriteControl(data); return;
		case 0x4005: pulse2.WriteSweep(data); return;
		case 0x4006: pulse2.WriteTimerLow(data); return;
		case 0x4007: pulse2.WriteTimerHigh(data); return;
		#pragma endregion
		#pragma region Triangle
		case 0x4008:
			triangle.lengthCounterEnabled = ((data >> 7) & 1);
			triangle.linearCounterPeriod = data & 0x7F;
			return;
		case 0x400A:
			triangle.timerPeriod = (triangle.timerPeriod & 0xFF00) | data;
			return;
		case 0x400B:
			if(triangle.enabled)
				triangle.lengthCounter = lengthTable[data >> 3];

			triangle.timerPeriod = (triangle.timerPeriod & 0x00FF) | ((data & 7) << 8);
			triangle.timer = triangle.timerPeriod;
			triangle.linearCounterReload = true;
			return;
		#pragma endregion
		#pragma region noise
		case 0x400C:
			noise.lengthCounterEnabled = ((data >> 5) & 1);
			noise.envelopeLoop = (data >> 5) & 1;

			noise.envelopeEnabled = ((data >> 4) & 1);
			noise.envelopePeriod = data & 0xF;
			noise.constantVolume = data & 0xF;
			noise.envelopeStart = true;
			return;
		case 0x400E:
			noise.mode = data & 0x80;
			noise.timerPeriod = noiseTable[data & 0xF];
			return;
		case 0x400F:
			if(noise.enabled)
				noise.lengthCounter = lengthTable[data >> 3];

			noise.envelopeStart = true;
			return;
		#pragma endregion
		#pragma region DMC
		case 0x4010:
			dmc.irqEnable = data & 0x80;
			dmc.loop = data & 0x40;
			dmc.timerPeriod = dmcTable[data & 0xF];
			dmc.irq &= dmc.irqEnable;
			return;
		case 0x4011:
			dmc.value = data & 0x7F;
			return;
		case 0x4012:
			dmc.sampleAddress = 0xC000 | (data << 6);
			return;
		case 0x4013:
			dmc.sampleLength = (data << 4) | 1;
			return;
		#pragma endregion
		case 0x4015: {
			pulse1.enabled = data & 0x1;
			pulse2.enabled = data & 0x2;
			triangle.enabled = data & 0x4;
			noise.enabled = data & 0x8;
			bool oldDmc = dmc.enabled;
			dmc.enabled = data & 0x10;

			if(!pulse1.enabled) {
				pulse1.lengthCounter = 0;
			}
			if(!pulse2.enabled) {
				pulse2.lengthCounter = 0;
			}
			if(!triangle.enabled) {
				triangle.lengthCounter = 0;
			}
			if(!noise.enabled) {
				noise.lengthCounter = 0;
			}
			if(dmc.enabled) {
				if(!oldDmc || dmc.bufferEmpty) {
					dmc.Reload();
					dmc.FillBuffer(*bus);
				}
			} else {
				dmc.currentLength = 0;
			}

			dmc.irq = false;
			return;
		}
		case 0x4017:
			last4017Write = data;
			frameCounterMode = data >> 7;

			if(frameCounter & 1) {
				frameCounter = -3;
			} else {
				frameCounter = -2;
			}

			IRQinhibit = (data >> 6) & 1;
			if(IRQinhibit) {
				Irq = false;
			}

			if(frameCounterMode) {
				ClockLength();
			}
			return;
	}

	if(vrc6) {
		switch(addr) {
			case 0x9000:
				vrc6Pulse1.volume = data & 0xF;
				vrc6Pulse1.dutyCycle = (data >> 4) & 7;
				vrc6Pulse1.mode = data >> 7;
				return;
			case 0x9001:
				vrc6Pulse1.timerPeriod = (vrc6Pulse1.timerPeriod & 0xFF00) | data;
				return;
			case 0x9002:
				vrc6Pulse1.timerPeriod = (vrc6Pulse1.timerPeriod & 0xFF) | ((data & 0xF) << 8);
				vrc6Pulse1.enabled = data >> 7;
				if(!vrc6Pulse1.enabled) {
					vrc6Pulse1.dutyValue = 15;
				}
				return;
			case 0xA000:
				vrc6Pulse2.volume = data & 0xF;
				vrc6Pulse2.dutyCycle = (data >> 4) & 7;
				vrc6Pulse2.mode = data >> 7;
				return;
			case 0xA001:
				vrc6Pulse2.timerPeriod = (vrc6Pulse2.timerPeriod & 0xFF00) | data;
				return;
			case 0xA002:
				vrc6Pulse2.timerPeriod = (vrc6Pulse2.timerPeriod & 0xFF) | ((data & 0xF) << 8);
				vrc6Pulse2.enabled = data >> 7;
				if(!vrc6Pulse2.enabled) {
					vrc6Pulse2.dutyValue = 15;
				}
				return;
			case 0xB000:
				vrc6Saw.accumRate = data & 0x3F;
				return;
			case 0xB001:
				vrc6Saw.timerPeriod = (vrc6Saw.timerPeriod & 0xFF00) | data;
				return;
			case 0xB002:
				vrc6Saw.timerPeriod = (vrc6Saw.timerPeriod & 0xFF) | ((data & 0xF) << 8);
				vrc6Saw.enabled = data >> 7;
				if(!vrc6Saw.enabled) {
					vrc6Saw.accumulator = 0;
				}
				return;
			case 0x9003:
				vrc6Halt = data & 1;
				if((data & 2) != 0) {
					vrc6FreqShift = 4;
				}
				if((data & 4) != 0) {
					vrc6FreqShift = 8;
				}
				return;
		}
	}
}

uint8_t RP2A03::ReadStatus(bool readOnly) {
	uint8_t res = 0;
	if(pulse1.lengthCounter > 0) {
		res |= 1 << 0;
	}
	if(pulse2.lengthCounter > 0) {
		res |= 1 << 1;
	}
	if(triangle.lengthCounter > 0) {
		res |= 1 << 2;
	}
	if(noise.lengthCounter > 0) {
		res |= 1 << 3;
	}
	if(dmc.currentLength > 0) {
		res |= 1 << 4;
	}

	if(Irq) {
		res |= 1 << 6;
	}
	if(dmc.irq) {
		res |= 1 << 7;
	}

	if(!readOnly) {
		Irq = false;
	}

	return res;
}

void RP2A03::ClockEnvelope() {
	pulse1.ClockEnvelope();
	pulse2.ClockEnvelope();

	if(triangle.linearCounterReload) {
		triangle.linearCounter = triangle.linearCounterPeriod;
	} else if(triangle.linearCounter > 0) {
		triangle.linearCounter--;
	}
	if(!triangle.lengthCounterEnabled) {
		triangle.linearCounterReload = false;
	}

	noise.ClockEnvelope();
}

void RP2A03::ClockLength() {
	pulse1.ClockLength();
	pulse2.ClockLength();
	triangle.ClockLength();
	noise.ClockLength();

	pulse1.ClockSweep();
	pulse2.ClockSweep();
}

void RP2A03::GenerateSample() {
	auto& buf = waveBuffer[bufferPos];

    // [0, 15]
	if(noise.enabled && noise.lengthCounter > 0 && (noise.shiftRegister & 1) == 0) {
		buf.noise = noise.envelopeEnabled ? noise.constantVolume : noise.envelopeVolume;
	} else {
		buf.noise = 0;
	}

    // [0, 127]
	buf.dmc = dmc.value;

    // [0, 0.741516245148]
	float tnd_out = 159.79f / (1.0f / ((triangleTable[triangle.dutyValue] / 8227.0f) + (buf.noise / 12241.0f) + (dmc.value / 22638.0f)) + 100);
    // [0, 0.258483105679]
	float pulse_out = 95.88f / (8128.0f / (pulse1.Output() + pulse2.Output()) + 100);
	float output = tnd_out + pulse_out; // between [0, 1]

	if(vrc6) {
        // [0, 15+15+31] = [0, 61]
        auto sum = vrc6Pulse1.Output() + vrc6Pulse2.Output() + vrc6Saw.Output(); // linear mixing

        // negative and scaled so the pulse channels are roughly the same volume as the default pulse channels
        // 30 / 0.258483105679
		output += -sum / 116.0617438466f;
	}

	Audio::PushSample(output);
	bufferPos++;
	if(bufferPos >= bufferLength) {
		// __debugbreak();
		throw std::runtime_error("Buffer overflow");
	}
}

bool RP2A03::GetIrq() const {
	return Irq || dmc.irq;
}

void RP2A03::SaveState(nlohmann::json& saver) const {
	saver["Irq"] = Irq;
	saver["last4017Write"] = last4017Write;
	saver["frameCounterMode"] = frameCounterMode;
	saver["frameCounter"] = frameCounter;
	saver["IRQinhibit"] = IRQinhibit;

	pulse1.SaveState(saver["pulse1"]);
	pulse2.SaveState(saver["pulse2"]);
	triangle.SaveState(saver["triangle"]);
	noise.SaveState(saver["noise"]);
	dmc.SaveState(saver["dmc"]);

	saver["vrc6"] = vrc6;
	saver["vrc6Halt"] = vrc6Halt;
	saver["vrc6FreqShift"] = vrc6FreqShift;

	vrc6Pulse1.SaveState(saver["vrc6Pulse1"]);
	vrc6Pulse2.SaveState(saver["vrc6Pulse2"]);
	vrc6Saw.SaveState(saver["vrc6Saw"]);

	assert(bufferPos == 0);
}

void RP2A03::LoadState(const nlohmann::json& saver) {
	Irq = saver["Irq"];
	last4017Write = saver["last4017Write"];
	frameCounterMode = saver["frameCounterMode"];
	frameCounter = saver["frameCounter"];
	IRQinhibit = saver["IRQinhibit"];

	pulse1.LoadState(saver["pulse1"]);
	pulse2.LoadState(saver["pulse2"]);
	triangle.LoadState(saver["triangle"]);
	noise.LoadState(saver["noise"]);
	dmc.LoadState(saver["dmc"]);

	vrc6 = saver["vrc6"];
	vrc6Halt = saver["vrc6Halt"];
	vrc6FreqShift = saver["vrc6FreqShift"];

	vrc6Pulse1.LoadState(saver["vrc6Pulse1"]);
	vrc6Pulse2.LoadState(saver["vrc6Pulse2"]);
	vrc6Saw.LoadState(saver["vrc6Saw"]);

	bufferPos = 0;
}

}
