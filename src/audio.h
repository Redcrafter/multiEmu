#pragma once

class RP2A03;

namespace Audio {
    // called at startup
    bool Init();
    // called at end
	void Dispose();

    // Called once per frame and generats 44100/60 = 735 samples
    void Resample(RP2A03& apu);
}
