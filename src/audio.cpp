#include "audio.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

#include <RtAudio.h>

#include "logger.h"

struct sample {
	float left;
	float right;

	sample& operator*=(const float val) {
		left *= val;
		right *= val;
		return *this;
	}

	friend sample operator*(const float b, const sample& a) {
		return { a.left * b, a.right * b };
	}

	friend sample operator+(const sample& a, const sample& b) {
		return { a.left + b.left, a.right + b.right };
	}
};

// normal audio quality 44100hz
constexpr uint32_t sampleRate = 44100;

// 8 frames worth of buffer
constexpr int bufferSize = 8 * 735;

// read position of output
static size_t readPos = 0;
// write position of input
static size_t writePos = 0;
// sample buffer
static std::array<sample, bufferSize> buffer;

// used to prevent popping
static sample lastSample { 0, 0 };

// samples added by PushSample
static std::vector<sample> inBuffer;

static std::unique_ptr<RtAudio> dac;
static bool audioRunning = false;

static int AudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) {
	const auto outBuffer = static_cast<sample*>(outputBuffer);

	if(readPos < writePos) {
		lastSample = buffer[(writePos - 1) % bufferSize];
	}

	size_t i = 0;
	// output number of requested samples
	for(; i < nBufferFrames && readPos < writePos; i++) {
		// copy buffer to left and right channel
		outBuffer[i] = buffer[readPos % bufferSize];
		readPos++;
	}

	// if we didn't have enough samples in the buffer
	if(i < nBufferFrames) {
		// Repeat last sample to prevent popping and fade out over time
		// so single frame drops don't pop and there's no noise over time
		for(; i < nBufferFrames; i++) {
			outBuffer[i] = lastSample;
			lastSample *= 0.9999;
		}
	}

	return 0;
}

bool Audio::Init() {
	dac = std::make_unique<RtAudio>();

	if(dac->getDeviceCount() >= 1) {
		RtAudio::StreamParameters parameters;
		parameters.deviceId = dac->getDefaultOutputDevice();
		parameters.nChannels = 2;
		parameters.firstChannel = 0;

		RtAudio::StreamOptions options;
		options.streamName = "MultiEmu";
		options.flags = RTAUDIO_MINIMIZE_LATENCY; // breaks mac?
		// TODO: test non fixed sample request

		uint32_t bufferFrames = sampleRate / 60;

		dac->openStream(&parameters, nullptr, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &AudioCallback, nullptr, &options);
		dac->startStream();

		audioRunning = true;
	} else {
		logger.Log("No audio device found.\n");
		audioRunning = false;
	}

	return audioRunning;
}

void Audio::Dispose() {
	if(dac->isStreamOpen()) {
		dac->stopStream();
		dac->closeStream();
	}
}

void Audio::Resample() {
	if(!audioRunning || inBuffer.empty()) {
		return;
	}

	auto ratio = inBuffer.size() / 735.0;

	// TODO: prevent aliasing?
	if(ratio > 1) {
		for(size_t i = 0; i < 735; i++) {
			buffer[writePos % bufferSize] = inBuffer[floor(i * ratio)];
			writePos++;
		}
	} else {
		// __debugbreak();
		// should not happen often
	}

	inBuffer.clear();
}

void Audio::PushSample(float value) {
	inBuffer.push_back({ value, value });
}

void Audio::PushSample(float left, float right) {
	inBuffer.push_back({ left, right });
}
