#pragma once
#include "../../saver.h"

namespace Nes {

class Bus;

struct SoundBase {
	bool enabled;
	bool lengthCounterEnabled = true;
	uint8_t lengthCounterPeriod;
	uint8_t lengthCounter;

	uint16_t timer = 0;
	uint16_t timerPeriod = 0;

	void ClockLength();
};

struct Envelope : SoundBase {
	bool envelopeEnabled;
	bool envelopeLoop;
	bool envelopeStart;
	uint8_t envelopePeriod;
	uint8_t envelopeValue;
	uint8_t envelopeVolume;
	uint8_t constantVolume;

	void ClockEnvelope();
};

struct Pulse : Envelope {
	uint8_t negative;

	uint8_t dutyCycle;
	uint8_t dutyValue;

	bool sweepReload;
	bool sweepEnabled;
	bool sweepNegate;
	uint8_t sweepShift;
	uint8_t sweepPeriod;
	uint8_t sweepValue;

	void WriteControl(uint8_t data);
	void WriteSweep(uint8_t data);
	void WriteTimerLow(uint8_t data);
	void WriteTimerHigh(uint8_t data);

	void Clock();
	void ClockSweep();

	uint8_t Output() const;
};

struct Triangle : SoundBase {
	uint8_t dutyValue;

	uint8_t linearCounterPeriod;
	uint8_t linearCounter = 0;
	bool linearCounterReload;

	void Clock();
};

struct Noise : Envelope {
	bool mode;
	uint16_t shiftRegister = 1;

	void Clock();
};

struct vrc6Pulse {
	bool enabled = false;
	uint8_t Volume = 0;

	uint8_t dutyCycle = 0;
	uint8_t dutyValue = 15;

	bool mode = false;

	uint16_t timer = 0;
	uint16_t timerPeriod = 0;

	void Clock(uint8_t freqShift);
	uint8_t Output();
};

struct vrc6Sawtooth {
	bool enabled = false;

	uint8_t step = 0;
	uint8_t accumulator = 0;
	uint8_t accumRate = 0;

	uint16_t timer = 0;
	uint16_t timerPeriod = 0;

	void Clock(uint8_t freqShift);
	uint8_t Output();
};

struct DMC {
	bool enabled;
	bool irq = false;

	bool irqEnable;
	bool loop;

	bool silence;

	uint8_t value = 0;
	uint16_t sampleAddress = 0;
	uint16_t sampleLength = 0;

	uint16_t currentAddress = 0;
	uint16_t currentLength = 0;

	bool bufferEmpty = true;
	uint8_t sampleBuffer = 0;

	uint8_t shiftRegister = 0;
	uint8_t bitCount = 1;

	uint16_t timer = 0;
	uint16_t timerPeriod = 0;

	void Clock(Bus& bus);
	void Reload();
	void FillBuffer(Bus& bus);
};

static constexpr int bufferLength = 32 * 1024; // 32 KiB

struct RP2A03state {
	bool Irq = false;

	uint8_t last4017Write = 0;
	bool frameCounterMode = false;
	int frameCounter = 0; // twice of what it should be
	bool IRQinhibit = false;

	Pulse pulse1, pulse2;
	Triangle triangle;
	Noise noise;
	DMC dmc;

	bool vrc6 = false;
	bool vrc6Halt = false;
	uint8_t vrc6FreqShift = 0;
	vrc6Pulse vrc6Pulse1;
	vrc6Pulse vrc6Pulse2;
	vrc6Sawtooth vrc6Saw;
};

class RP2A03 : public RP2A03state {
	friend class ApuWindow;
	friend class Core;

  private:
	Bus* bus = nullptr;

	int bufferPos = 0;
	int lastBufferPos = 0;
	struct {
		uint8_t noise, dmc;
	} waveBuffer[bufferLength];

  public:
	RP2A03(Bus* bus);

	void Clock();
	void Reset();

	void CpuWrite(uint16_t addr, uint8_t data);
	uint8_t ReadStatus(bool readOnly);

	void ClockEnvelope();
	void ClockLength();

	void GenerateSample();
	bool GetIrq();

	void SaveState(saver& saver);
	void LoadState(saver& saver);
};

}
