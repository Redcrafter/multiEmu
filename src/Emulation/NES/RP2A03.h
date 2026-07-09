#pragma once
#include <nlohmann/json.hpp>

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

	void SaveBase(nlohmann::json& saver) const {
		saver["enabled"] = enabled;
		saver["lengthCounterEnabled"] = lengthCounterEnabled;
		saver["lengthCounterPeriod"] = lengthCounterPeriod;
		saver["lengthCounter"] = lengthCounter;
		saver["timer"] = timer;
		saver["timerPeriod"] = timerPeriod;
	}

	void LoadBase(const nlohmann::json& saver) {
		enabled = saver["enabled"];
		lengthCounterEnabled = saver["lengthCounterEnabled"];
		lengthCounterPeriod = saver["lengthCounterPeriod"];
		lengthCounter = saver["lengthCounter"];
		timer = saver["timer"];
		timerPeriod = saver["timerPeriod"];
	}
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

	void SaveEnv(nlohmann::json& saver) const {
		saver["envelopeEnabled"] = envelopeEnabled;
		saver["envelopeLoop"] = envelopeLoop;
		saver["envelopeStart"] = envelopeStart;
		saver["envelopePeriod"] = envelopePeriod;
		saver["envelopeValue"] = envelopeValue;
		saver["envelopeVolume"] = envelopeVolume;
		saver["constantVolume"] = constantVolume;
	}
	void LoadEnv(const nlohmann::json& saver) {
		envelopeEnabled = saver["envelopeEnabled"];
		envelopeLoop = saver["envelopeLoop"];
		envelopeStart = saver["envelopeStart"];
		envelopePeriod = saver["envelopePeriod"];
		envelopeValue = saver["envelopeValue"];
		envelopeVolume = saver["envelopeVolume"];
		constantVolume = saver["constantVolume"];
	}
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

	void SaveState(nlohmann::json& saver) const;
	void LoadState(const nlohmann::json& saver);
};

struct Triangle : SoundBase {
	uint8_t dutyValue;

	uint8_t linearCounterPeriod;
	uint8_t linearCounter = 0;
	bool linearCounterReload;

	void Clock();

	void SaveState(nlohmann::json& saver) const;
	void LoadState(const nlohmann::json& saver);
};

struct Noise : Envelope {
	bool mode;
	uint16_t shiftRegister = 1;

	void Clock();

	void SaveState(nlohmann::json& saver) const {
		SaveBase(saver);
		SaveEnv(saver);
		saver["mode"] = mode;
		saver["shiftRegister"] = shiftRegister;
	}
	void LoadState(const nlohmann::json& saver) {
		LoadBase(saver);
		LoadEnv(saver);
		mode = saver["mode"];
		shiftRegister = saver["shiftRegister"];
	}
};

struct vrc6Pulse {
	bool enabled = false;
	uint8_t volume = 0;

	uint8_t dutyCycle = 0;
	uint8_t dutyValue = 15;

	bool mode = false;

	uint16_t timer = 0;
	uint16_t timerPeriod = 0;

	void Clock(uint8_t freqShift);
	uint8_t Output();

	void SaveState(nlohmann::json& saver) const {
		saver["enabled"] = enabled;
		saver["volume"] = volume;
		saver["dutyCycle"] = dutyCycle;
		saver["dutyValue"] = dutyValue;
		saver["mode"] = mode;
		saver["timer"] = timer;
		saver["timerPeriod"] = timerPeriod;
	}
	void LoadState(const nlohmann::json& saver) {
		enabled = saver["enabled"];
		volume = saver["volume"];
		dutyCycle = saver["dutyCycle"];
		dutyValue = saver["dutyValue"];
		mode = saver["mode"];
		timer = saver["timer"];
		timerPeriod = saver["timerPeriod"];
	}
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

	void SaveState(nlohmann::json& saver) const {
		saver["enabled"] = enabled;
		saver["step"] = step;
		saver["accumulator"] = accumulator;
		saver["accumRate"] = accumRate;
		saver["timer"] = timer;
		saver["timerPeriod"] = timerPeriod;
	}
	void LoadState(const nlohmann::json& saver) {
		enabled = saver["enabled"];
		step = saver["step"];
		accumulator = saver["accumulator"];
		accumRate = saver["accumRate"];
		timer = saver["timer"];
		timerPeriod = saver["timerPeriod"];
	}
};

struct DMC {
	bool enabled;
	bool irq;

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

class RP2A03 {
	friend class ApuWindow;
	friend class Core;

	bool Irq;

	uint8_t last4017Write;
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
	bool GetIrq() const;

	void SaveState(nlohmann::json& saver) const;
	void LoadState(const nlohmann::json& saver);
};

}
