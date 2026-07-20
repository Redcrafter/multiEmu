#include "audio.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

// #include <RtAudio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "logger.h"

struct sample {
	float left;
	float right;
};

// normal audio quality 44100hz
constexpr uint32_t sampleRate = 44100;

// samples added by PushSample
static std::vector<sample> inBuffer;

static SDL_AudioStream* dac;

static sample lastSample;

void audioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
	// logger.Log("%i %i\n", additional_amount, total_amount);
	auto req = additional_amount / sizeof(sample);

	// not enough audio for frame
	// repeat last sample to prevent popping
	if(req > 0) {
		std::array<sample, 735> buf;
		buf.fill(lastSample);
		SDL_PutAudioStreamData(dac, buf.data(), sizeof(sample) * std::min(buf.size(), req));
	}
}

bool Audio::Init() {
	SDL_AudioSpec want;
	SDL_zero(want);
	want.format = SDL_AUDIO_F32;
	want.channels = 2;
	want.freq = sampleRate;

	dac = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, &audioCallback, nullptr);
	SDL_ResumeAudioStreamDevice(dac);

	return true;
}

void Audio::Dispose() {
	if(dac) {
		SDL_DestroyAudioStream(dac);
	}
}

void Audio::Resample() {
	if(dac == nullptr || inBuffer.empty()) {
		return;
	}

	auto ratio = inBuffer.size() / (sampleRate / 60.0f);

	std::array<sample, 735> buffer;
	for(size_t i = 0; i < 735; i++) {
		buffer[i] = inBuffer[floor(i * ratio)];
	}
	lastSample = buffer[buffer.size() - 1];

	SDL_PutAudioStreamData(dac, buffer.data(), sizeof(sample) * 735);
	inBuffer.clear();
}

void Audio::PushSample(float value) {
	inBuffer.push_back({ value, value });
}

void Audio::PushSample(float left, float right) {
	inBuffer.push_back({ left, right });
}
