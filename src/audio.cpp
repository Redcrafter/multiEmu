#include "audio.h"

#include <cstdint>
#include <memory>

#include <RtAudio.h>

#include "Emulation/NES/RP2A03.h"
#include "logger.h"

// normal audio quality 44100hz
static const uint32_t sampleRate = 44100;

// 8 frames worth of buffer
const int bufferSize = 8 * 735;

// read position of output
static size_t readPos = 0;
// write position of input
static size_t writePos = 0;
// sample buffer
static float buffer[bufferSize];

static std::unique_ptr<RtAudio> dac;
static bool audioRunning = false;

static int AudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) {
	auto outBuffer = static_cast<float*>(outputBuffer);

	size_t i = 0;
	// output number of requested samples
	for(; i < nBufferFrames && readPos < writePos; i++) {
		// copy buffer to left and right channel
		outBuffer[i * 2] = outBuffer[i * 2 + 1] = buffer[readPos % bufferSize];
		readPos++;
	}

	// if we didn't have enough samples in the buffer
	if(i < nBufferFrames) {
		// Repeat last sample to prevent popping
		int lastPos = readPos - 1;
		if(lastPos < 0) {
			lastPos = 0;
		}
		auto last = buffer[lastPos % bufferSize];
		for(size_t i = 0; i < nBufferFrames; ++i) {
			outBuffer[i * 2] = outBuffer[i * 2 + 1] = last;
		}
	}

	return 0;
}

bool Audio::Init() {
	try {
		dac = std::make_unique<RtAudio>();

		if(dac->getDeviceCount() >= 1) {
			RtAudio::StreamParameters parameters;
			parameters.deviceId = dac->getDefaultOutputDevice();
			parameters.nChannels = 2;
			parameters.firstChannel = 0;

			RtAudio::StreamOptions options;
			// options.flags = RTAUDIO_MINIMIZE_LATENCY; // breaks mac TODO: test non fixed sample request

			uint32_t bufferFrames = sampleRate / 60;

			dac->openStream(&parameters, nullptr, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &AudioCallback, nullptr, &options);
			dac->startStream();

			audioRunning = true;
		} else {
			logger.Log("No audio device found.\n");

			audioRunning = false;
		}
	} catch(RtAudioError & e) {
		logger.Log("Failed to initialize audio driver: %s\n", e.what());

		audioRunning = false;
	}

	return audioRunning;
}

void Audio::Dispose() {
	try {
		dac->stopStream();
	} catch(RtAudioError & e) {
		logger.Log("Failed to stop audio stream: %s\n", e.what());
	}

	if(dac->isStreamOpen()) {
		dac->closeStream();
	}
}

void Audio::Resample(RP2A03& apu) {
	if(!audioRunning) {
		apu.bufferPos = 0;
		return;
	}
	const int samples = apu.bufferPos;
	// printf("Buffer usage %i\n", samples);

	// how many samples to merge
	const auto frac = samples / 735.0;
	// position in apu's buffer
	int readPos = 0;
	// used to account for sample fractions
	float mapped = 0;

	// TODO: fancy linear interpolation?
	for(int i = 0; i < 735 && readPos < samples; i++) {
		float sample = 0;
		int count = 0;

		// take frac samples from waveBuffer and average them
		while(mapped <= frac && readPos < samples) {
			mapped++;
			sample += apu.waveBuffer[readPos].sample;
			readPos++;
			count++;
		}
		mapped -= frac;

		buffer[writePos % bufferSize] = (sample / count);
		writePos++;
	}
	
	// copy number of samples for visualization
	apu.lastBufferPos = apu.bufferPos;
	// reset apu buffer
	apu.bufferPos = 0;
}
