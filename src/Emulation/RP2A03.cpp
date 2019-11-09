#include "RP2A03.h"
#include "Bus.h"
#include <cassert>

const int lengthTable[] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};
const int dutyTable[4][8] = {
	{0, 1, 0, 0, 0, 0, 0, 0},
	{0, 1, 1, 0, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0},
	{1, 0, 0, 1, 1, 1, 1, 1},
};

const int triangleTable[] = {
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};
const int noiseTable[] = {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,};
const int dmcTable[] = {214, 190, 170, 160, 143, 127, 113, 107, 95, 80, 71, 64, 53, 42, 36, 27,};

const float pulseTable[] = {0, 0.011609139523578026, 0.022939481268011527, 0.03400094921689606, 0.04480300187617261, 0.05535465924895688, 0.06566452795600367, 0.07574082464884459, 0.08559139784946236, 0.09522374833850243, 0.10464504820333041, 0.11386215864759427, 0.12288164665523155, 0.13170980059397538, 0.14035264483627205, 0.1488159534690486, 0.15710526315789472, 0.16522588522588522, 0.1731829170024174, 0.18098125249301955, 0.18862559241706162, 0.19612045365662886, 0.20347017815646784, 0.21067894131185272, 0.21775075987841944, 0.2246894994354535, 0.2314988814317673, 0.23818248984115256, 0.2447437774524158, 0.2511860718171926, 0.25751258087706685};
const float tndTable[] = {0, 0.006699823979696262, 0.01334502018019487, 0.01993625400950099, 0.026474180112418616, 0.032959442587297105, 0.03939267519756107, 0.04577450157816932, 0.05210553543714433, 0.05838638075230885, 0.06461763196336215, 0.07079987415942428, 0.07693368326217241, 0.08301962620468999, 0.08905826110614481, 0.09505013744240969, 0.10099579621273477, 0.10689577010257789, 0.11275058364269584, 0.11856075336459644, 0.12432678795244785, 0.1300491883915396, 0.13572844811338536, 0.1413650531375568, 0.1469594822103333, 0.15251220694025122, 0.15802369193063237, 0.16349439490917161, 0.16892476685465738, 0.1743152521209005, 0.1796662885579421, 0.18497830763060993, 0.19025173453449087, 0.19548698830938505, 0.20068448195030472, 0.20584462251608032, 0.2109678112356332, 0.2160544436119733, 0.2211049095239788, 0.22611959332601225, 0.2310988739454269, 0.23604312497801538, 0.24095271478145042, 0.24582800656676793, 0.25066935848793903, 0.25547712372957787, 0.2602516505928307, 0.26499328257948945, 0.26970235847437257, 0.27437921242601526, 0.27902417402570834, 0.28363756838492643, 0.2882197162111822, 0.292770933882345, 0.29729153351945914, 0.3017818230580978, 0.3062421063182866, 0.31067268307302937, 0.31507384911547015, 0.3194458963247213, 0.32378911273039, 0.3281037825758322, 0.3323901863801631, 0.33664860099905314, 0.3408792996843372, 0.34508255214246325, 0.349258624591807, 0.3534077798188791, 0.3575302772334479, 0.36162637292260397, 0.3656963197037888, 0.3697403671768112, 0.3737587617748739, 0.37775174681463214, 0.38171956254530554, 0.38566244619686446, 0.3895806320273106, 0.3934743513690717, 0.3973438326745308, 0.40118930156070615, 0.405010980853104, 0.4088090906287582, 0.41258384825847705, 0.4163354684483128, 0.42006416328027124, 0.4237701422522769, 0.42745361231741014, 0.4311147779224318, 0.4347538410456096, 0.43837100123386197, 0.4419664556392331, 0.44554039905471293, 0.44909302394941686, 0.4526245205031371, 0.45613507664027986, 0.4596248780632002, 0.4630941082849479, 0.4665429486614358, 0.46997157842304194, 0.47338017470565896, 0.4767689125811996, 0.48013796508757145, 0.48348750325813084, 0.48681769615062515, 0.49012871087563703, 0.493420712624537, 0.49669386469695664, 0.49994832852779125, 0.5031842637137408, 0.5064018280393993, 0.5096011775029012, 0.5127824663411329, 0.5159458470545188, 0.5190914704313901, 0.5222194855719443, 0.5253300399118033, 0.528423279245178, 0.5314993477476477, 0.5345583879985607, 0.5376005410030638, 0.5406259462137686, 0.5436347415520602, 0.5466270634290563, 0.5496030467662235, 0.5525628250156552, 0.5555065301800212, 0.5584342928321915, 0.5613462421345432, 0.5642425058579547, 0.5671232104004943, 0.5699884808058077, 0.5728384407812124, 0.5756732127155, 0.5784929176964575, 0.5812976755281083, 0.5840876047476803, 0.5868628226423054, 0.5896234452654553, 0.5923695874531196, 0.595101362839729, 0.5978188838738291, 0.6005222618335111, 0.6032116068415997, 0.6058870278806079, 0.6085486328074569, 0.6111965283679723, 0.6138308202111536, 0.6164516129032258, 0.6190590099414757, 0.6216531137678758, 0.6242340257825014, 0.6268018463567424, 0.6293566748463153, 0.6318986096040777, 0.6344277479926501, 0.6369441863968464, 0.6394480202359187, 0.6419393439756177, 0.6444182511400732, 0.6468848343234979, 0.6493391852017159, 0.6517813945435207, 0.6542115522218658, 0.6566297472248885, 0.659036067666773, 0.6614306007984521, 0.6638134330181533, 0.6661846498817908, 0.6685443361132047, 0.670892575614252, 0.6732294514747513, 0.6755550459822829, 0.6778694406318475, 0.6801727161353863, 0.6824649524311629, 0.684746228693012, 0.6870166233394548, 0.6892762140426848, 0.6915250777374256, 0.693763290629662, 0.6959909282052493, 0.6982080652383982, 0.7004147758000423, 0.7026111332660865, 0.70479721032554, 0.7069730789885358, 0.7091388105942369, 0.7112944758186339, 0.7134401446822323, 0.7155758865576349, 0.7177017701770176, 0.7198178636395035, 0.7219242344184336, 0.7240209493685391, 0.7261080747330146, 0.7281856761504939, 0.7302538186619317, 0.7323125667173908, 0.734361984182737, 0.7364021343462434, 0.7384330799251054, 0.7404548830718675, 0.742467605380763};

void Pulse::Clock() {
	if(timer == 0) {
		timer = timerPeriod;

		dutyValue = (dutyValue + 1) % 8;
	} else {
		timer--;
	}
}

void Triangle::Clock() {
	if(timer == 0) {
		timer = timerPeriod;

		if(linearCounter > 0 && lengthCounter > 0) {
			dutyValue = (dutyValue + 1) % 32;
		}
	} else {
		timer--;
	}
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

void Stuff::ClockLength() {
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

uint8_t Pulse::Output() {
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

	//if(timer < 8) {
	//	return 0;
	//}

	if(envelopeEnabled) {
		return constantVolume;
	} else {
		return envelopeVolume;
	}
}

void DMC::Clock(Bus* bus) {
	if(timer > 0) {
		timer--;
		return;
	}
	timer = timerPeriod;

	if(!silence) {
		if(shiftRegister & 1) {
			if(value <= 125) {
				value += 2;
			}
		} else {
			if(value >= 2) {
				value -= 2;
			}
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

void DMC::FillBuffer(Bus* bus) {
	if(bufferEmpty && currentLength > 0) {
		// TODO: Stall cpu for 4 cycles
		sampleBuffer = bus->CpuRead(currentAddress);
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

		// printf("Current length: %i\n", currentLength);
	}
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
				// break;
			case 29828:
			case 29830:
				if(!IRQinhibit) {
					Irq = true;
				}
				break;
		}
	}

	GenerateSample();

	if(frameCounter % 2 == 0) {
		pulse1.Clock();
		pulse2.Clock();
		noise.Clock();
		dmc.Clock(bus);
	}
	triangle.Clock();

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

	CpuWrite(0x4017, last4017Write);
	frameCounter += 10;
	Irq = false;

	pulse1.lengthCounterEnabled = true;
	pulse2.lengthCounterEnabled = true;
	triangle.lengthCounterEnabled = true;
	noise.lengthCounterEnabled = true;
}

void RP2A03::CpuWrite(uint16_t addr, uint8_t data) {
	switch(addr) {
			#pragma region Pulse
		case 0x4000:
			pulse1.WriteControl(data);
			break;
		case 0x4001:
			pulse1.WriteSweep(data);
			break;
		case 0x4002:
			pulse1.WriteTimerLow(data);
			break;
		case 0x4003:
			pulse1.WriteTimerHigh(data);
			break;
		case 0x4004:
			pulse2.WriteControl(data);
			break;
		case 0x4005:
			pulse2.WriteSweep(data);
			break;
		case 0x4006:
			pulse2.WriteTimerLow(data);
			break;
		case 0x4007:
			pulse2.WriteTimerHigh(data);
			break;
			#pragma endregion
			#pragma region Triangle
		case 0x4008:
			triangle.lengthCounterEnabled = ((data >> 7) & 1);
			triangle.linearCounterPeriod = data & 0x7F;
			break;
		case 0x400A:
			triangle.timerPeriod = (triangle.timerPeriod & 0xFF00) | data;
			break;
		case 0x400B:
			if(triangle.enabled)
				triangle.lengthCounter = lengthTable[data >> 3];

			triangle.timerPeriod = (triangle.timerPeriod & 0x00FF) | ((data & 7) << 8);
			triangle.timer = triangle.timerPeriod;
			triangle.linearCounterReload = true;
			break;
			#pragma endregion
			#pragma region noise
		case 0x400C:
			noise.lengthCounterEnabled = ((data >> 5) & 1);
			noise.envelopeLoop = (data >> 5) & 1;

			noise.envelopeEnabled = ((data >> 4) & 1);
			noise.envelopePeriod = data & 0xF;
			noise.constantVolume = data & 0xF;
			noise.envelopeStart = true;
			break;
		case 0x400E:
			noise.mode = data & 0x80;
			noise.timerPeriod = noiseTable[data & 0xF];
			break;
		case 0x400F:
			if(noise.enabled)
				noise.lengthCounter = lengthTable[data >> 3];

			noise.envelopeStart = true;
			break;
			#pragma endregion
		case 0x4010:
			dmc.irqEnable = data & 0x80;
			dmc.loop = data & 0x40;
			dmc.timerPeriod = dmcTable[data & 0xF];

			if(!dmc.irqEnable) {
				dmc.irq = false;
			}
			break;
		case 0x4011:
			dmc.value = data & 0x7F;
			break;
		case 0x4012:
			dmc.sampleAddress = 0xC000 | (data << 6);
			break;
		case 0x4013:
			dmc.sampleLength = (data << 4) | 1;

			// dmc.Reload();
			break;
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
				if(!oldDmc || dmc.currentLength == 0) {
					dmc.Reload();
				}
				dmc.FillBuffer(bus);
			} else {
				dmc.currentLength = 0;
			}

			dmc.irq = false;
			break;
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
			break;
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

float RP2A03::GenerateSample() {
	auto& buf = waveBuffer[bufferPos];

	buf.pulse1 = pulse1.Output();
	buf.pulse2 = pulse2.Output();

	uint8_t tnd = 0;
	if(triangle.enabled && triangle.lengthCounter > 0 && triangle.linearCounter > 0) {
		buf.triangle = triangleTable[triangle.dutyValue] * 3;
		tnd += buf.triangle;
	} else {
		buf.triangle = 0;
	}

	if(noise.enabled && noise.lengthCounter > 0 && (noise.shiftRegister & 1) == 0) {
		if(noise.envelopeEnabled) {
			buf.noise = noise.constantVolume * 2;
		} else {
			buf.noise = noise.envelopeVolume * 2;
		}
		tnd += buf.noise;
	} else {
		buf.noise = 0;
	}

	tnd += dmc.value;

	const auto val = pulseTable[buf.pulse1 + buf.pulse2] + tndTable[tnd];

	buf.sample = val;
	bufferPos++;
	if(bufferPos >= bufferLength) {
		// __debugbreak();
		throw std::runtime_error("Buffer overflow");
	}

	return val;
}

bool RP2A03::GetIrq() {
	return Irq || dmc.irq;
}

void RP2A03::SaveState(saver& saver) {
	saver.Write(reinterpret_cast<const char*>(this), sizeof(RP2A03state));
	assert(bufferPos == 0);
}

void RP2A03::LoadState(saver& saver) {
	saver.Read(reinterpret_cast<char*>(this), sizeof(RP2A03state));
	bufferPos = 0;
}
