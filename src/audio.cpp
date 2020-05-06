#include "audio.h"
#include "logger.h"
#include "Emulation/RP2A03.h"

#include <stdint.h>
#include <RtAudio.h>
#include <memory>

const int bufferSize = 8 * 735;

static size_t readPos = 0;
static size_t writePos = 0;
static float buffer[bufferSize];

static std::unique_ptr<RtAudio> dac;
static bool audioRunning = false;

static int AudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) {
	auto outBuffer = static_cast<float*>(outputBuffer);

	int i = 0;
	for(; i < nBufferFrames && readPos < writePos; i++) {
		outBuffer[i * 2] = outBuffer[i * 2 + 1] = buffer[readPos % bufferSize];
		readPos++;
	}

	if(i < nBufferFrames) {
		// Repeat last sample to prevent popping
		int lastPos = readPos - 1;
		if(lastPos < 0) {
			lastPos = 0;
		}
		auto last = buffer[lastPos % bufferSize];
		for(int i = 0; i < nBufferFrames; ++i) {
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
			// options.flags = RTAUDIO_MINIMIZE_LATENCY; // breaks mac TODO: handle non fixed sample request

			uint32_t sampleRate = 44100;
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

	const auto frac = samples / 735.0;
	int readPos = 0;
	float mapped = 0;

	for(int i = 0; i < 735 && readPos < samples; i++) {
		float sample = 0;
		int count = 0;

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
	
	apu.lastBufferPos = apu.bufferPos;
	apu.bufferPos = 0;
}
