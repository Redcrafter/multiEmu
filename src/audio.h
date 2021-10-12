#pragma once

namespace Audio {

// called at startup
bool Init();
// called at end
void Dispose();

// Called once per frame and generates 44100/60 = 735 samples
void Resample();

// push samples onto buffer. should be close to 735 samples/frame to reduce computation. buffer gets resampled to 735 samples
void PushSample(float value);

// push samples onto buffer. should be close to 735 samples/frame to reduce computation. buffer gets resampled to 735 samples
void PushSample(float left, float right);

}
