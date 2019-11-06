#pragma once
#include <cstdint>
#include "../saver.h"

class Bus;

struct Stuff {
	bool enabled;
	bool lengthCounterEnabled;
	uint8_t lengthCounterPeriod;
	uint8_t lengthCounter;

	uint16_t timer;
	uint16_t timerPeriod;

	void ClockLength();
};

struct Envelope : Stuff {
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

	uint8_t Output();
};

struct Triangle : Stuff {
	uint8_t dutyValue;

	uint8_t linearCounterPeriod;
	uint8_t linearCounter;
	bool linearCounterReload;

	void Clock();
};

struct Noise : Envelope {
	bool mode;
	uint16_t shiftRegister = 1;

	void Clock();
};

struct DMC {
	DMC(Bus* bus);

	Bus* bus;

	bool enabled;
	bool irq;

	bool irqEnable;
	bool loop;

	bool silence;

	uint8_t value = 0;
	uint16_t sampleAddress;
	uint16_t sampleLength;

	uint16_t currentAddress;
	uint16_t currentLength;

	bool bufferEmpty;
	uint8_t sampleBuffer;

	uint8_t shiftRegister;
	uint8_t bitCount;

	uint16_t timer;
	uint16_t timerPeriod;

	void Clock();
};

class RP2A03 {
private:
	static const int bufferLength = 1024 * 1024;
	bool Irq = false;

	uint8_t last4017Write = 0;
	bool frameCounterMode = false;
	int frameCounter = 0; // twice of what it should be
	bool IRQinhibit = false;

	Pulse pulse1, pulse2;
	Triangle triangle;
	Noise noise;
	DMC dmc;
public:
	RP2A03(Bus* bus);

	void Clock();
	void Reset();

	void CpuWrite(uint16_t addr, uint8_t data);
	uint8_t CpuRead(uint16_t addr);

	void ClockEnvelope();
	void ClockLength();

	float GenerateSample();
	bool GetIrq();

	void SaveState(saver& saver);
	void LoadState(saver& saver);

	int bufferPos = 0;
	struct {
		float sample;
		uint8_t pulse1, pulse2, triangle, noise;
	} waveBuffer[bufferLength]; // 1 MiB
};
