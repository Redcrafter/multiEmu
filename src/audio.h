#pragma once

class RP2A03;

namespace Audio {
    bool Init();
	void Dispose();

    void Resample(RP2A03& apu);
}
