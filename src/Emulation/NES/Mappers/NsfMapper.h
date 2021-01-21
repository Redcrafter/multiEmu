#pragma once
#include "Mapper.h"

namespace Nes {

struct NsfFormat {
	char format[5];

	uint8_t version;
	uint8_t numSongs;
	uint8_t startSong;

	uint16_t loadAddress;
	uint16_t initAddress;
	uint16_t playAddress;

	char songName[32];
	char artist[32];
	char copyright[32];

	uint16_t playSpeedNtsc;

	uint8_t bankInit[8];

	uint16_t playSpeedPal;
	bool palNtsc;

	struct {
		bool vrc6 : 1;
		bool vrc7 : 1;
		bool fds : 1;
		bool mmc5 : 1;
		bool namco163 : 1;
		bool sunsoft5B : 1;

		uint8_t reserved : 2;
	} extraSoundChip;

	uint32_t length;
	std::vector<uint8_t> rom;

	NsfFormat(const std::string& path);
};

class NsfMapper : public Mapper {
public:
	NsfFormat nsf;
private:
	// Whether the NSF is bankswitched
	bool BankSwitched;
	// the bankswitch values to be used before the INIT routine is called
	uint8_t InitBankSwitches[8];
	// An image of the entire PRG space where the unmapped files are located
	uint8_t FakePRG[32768];
	
	// PRG bankswitching
	int prg_banks_4k[8];
	// whether vectors are currently patched. they should not be patched when running init/play routines because data from the ends of banks might get used
	bool Patch_Vectors = true;
	// Current 1-indexed song number (1 is the first song)
	int CurrentSong = 1;
	// Whether the INIT routine needs to be called
	bool InitPending = true;
	// Previous button state for button press handling
	int ButtonState;

	uint8_t NSFROM[0x23] = {
		//@NMIVector
		//Suspend vector patching
		//3800:LDA $3FF3
		0xAD,0xF3,0x3F,

		//Initialize stack pointer
		//3803:LDX #$FF
		0xA2,0xFF,
		//3805:TXS
		0x9A,

		//Check (and clear) InitPending flag
		//3806:LDA $3FF0
		0xAD,0xF0,0x3F,
		//3809:BEQ $8014
		0xF0,0x09,

		//Read the next song (resetting the player) and PAL flag into A and X and then call the INIT routine
		//380B:LDA $3FF1 
		0xAD,0xF1,0x3F,
		//380E:LDX $3FF2
		0xAE,0xF2,0x3F,
		//3811:JSR INIT
		0x20,0x00,0x00,

		//Fall through to:
		//@Play - call PLAY routine with X and Y cleared (this is not supposed to be required, but fceux did it)
		//3814:LDA #$00 
		0xA9,0x00,
		//3816:TAX
		0xAA,
		//3817:TAY
		0xA8,
		//3818:JSR PLAY
		0x20,0x00,0x00,

		//Resume vector patching and infinite loop waiting for next NMI
		//381B:LDA $3FF4
		0xAD,0xF4,0x3F,
		//381E:BCC $XX1E
		0x90,0xFE,

		//@ResetVector - just set up an infinite loop waiting for the first NMI
		//3820:CLC
		0x18,
		//3821:BCC $XX24 
		0x90,0xFE,
	};

public:
	NsfMapper(const std::string& path);
	// NsfMapper(const NsfFormat header);

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;
	bool ppuWrite(uint16_t addr, uint8_t data) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;
};

}
