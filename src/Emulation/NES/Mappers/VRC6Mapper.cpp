#include "VRC6Mapper.h"

namespace Nes {

static uint8_t Banks[16 * 16]; // which of the 8 chr regs is used to determine the bank here?
static uint8_t Masks[16 * 16]; // what is the resulting 8 bit chr reg value ANDed with?
static uint8_t A10s[16 * 16];  // and then what is it ORed with?

static uint8_t PTables[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x80, 0xc0, 0x81, 0xc1, 0x82, 0xc2, 0x83, 0xc3,
	0x00, 0x01, 0x02, 0x03, 0x84, 0xc4, 0x85, 0xc5,
};

static void GetBankByte(int b003, int banknum, uint8_t& bank, uint8_t& mask, uint8_t& a10) {
	if(banknum < 8) { // pattern tables
		int ptidx = b003 & 3;
		if(ptidx == 3) ptidx--;
		auto pt = PTables[ptidx * 8 + banknum];

		bank = pt & 7;
		mask = (pt & 0x80) ? 0xfe : 0xff;
		a10 = ((pt & 0x80) && (pt & 0x40)) ? 1 : 0;
	} else { // nametables
		banknum &= 3;
		switch(b003 & 7) {
			case 0:
			case 6:
			case 7: // H-mirror, 6677
				bank = (banknum >> 1) | 6;
				break;
			case 2:
			case 3:
			case 4: // V-mirror, 6767
				bank = banknum | 6;
				break;
			case 1:
			case 5: // 4 screen, 4567
			default:
				bank = banknum | 4;
				break;
		}
		switch(b003) {
			case 0:
			case 7: // V-mirror
				mask = 0xfe;
				a10 = banknum & 1;
				break;
			case 3:
			case 4: // H-mirror
				mask = 0xfe;
				a10 = banknum >> 1;
				break;
			case 8:
			case 15: // 1scA
				mask = 0xfe;
				a10 = 0;
				break;
			case 11:
			case 12: // 1scB
				mask = 0xfe;
				a10 = 1;
				break;
			default: // no replacement
				mask = 0xff;
				a10 = 0;
				break;
		}
	}
}

VRC6Mapper::VRC6Mapper(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr, bool swap) : Mapper(prg, chr), swap(swap) {
	prgBanks = prg.size() / 0x2000;
	chrBanks = chr.size() / 0x400;

	prgBankOffset[0] = 0;
	prgBankOffset[1] = 0;
	prgBankOffset[2] = 0;
	prgBankOffset[3] = (prgBanks - 1) * 0x2000; // fixed last bank

	std::ranges::fill(chr_banks_1k, 0);
	std::ranges::fill(vram, 0);

	int idx = 0;
	for(int b003 = 0; b003 < 16; b003++) {
		for(int banknum = 0; banknum < 16; banknum++) {
			uint8_t bank = 0, mask = 0, a10 = 0;
			GetBankByte(b003, banknum, bank, mask, a10);
			Banks[idx] = bank;
			Masks[idx] = mask;
			A10s[idx] = a10;
			idx++;
		}
	}
}

int VRC6Mapper::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[(addr & 0x1FFF) | prgBankOffset[(addr >> 13) & 3]];
		return true;
	} else if(addr >= 0x6000 && ramEnable) {
		data = prgRam[addr & 0x1FFF];
		return true;
	}
	return false;
}

bool VRC6Mapper::cpuWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x6000 && addr < 0x8000 && ramEnable) {
		prgRam[addr & 0x1FFF] = data;
		return true;
	}

	switch(addr & 0xF003) {
		case 0x8000:
		case 0x8001:
		case 0x8002:
		case 0x8003:
			data = data & 0x0F;
			prgBankOffset[0] = data * 0x4000;
			prgBankOffset[1] = data * 0x4000 + 0x2000;
			return true;
		case 0xB003:
			PPUBankingMode = data & 0xF;
			NTROM = (data >> 4) & 1;
			chrA10replace = (data >> 5) & 1;
			ramEnable = data >> 7;
			return true;
		case 0xC000:
		case 0xC001:
		case 0xC002:
		case 0xC003:
			prgBankOffset[2] = (data & 0x1F) * 0x2000;
			return true;
		case 0xD000: chr_banks_1k[0] = data; return true;
		case 0xD001: chr_banks_1k[1] = data; return true;
		case 0xD002: chr_banks_1k[2] = data; return true;
		case 0xD003: chr_banks_1k[3] = data; return true;
		case 0xE000: chr_banks_1k[4] = data; return true;
		case 0xE001: chr_banks_1k[5] = data; return true;
		case 0xE002: chr_banks_1k[6] = data; return true;
		case 0xE003: chr_banks_1k[7] = data; return true;

		case 0xF000:
			irq_reload = data;
			return true;
		case 0xF001:
			irq_mode = (data >> 2) & 1;
			irq_autoen = data & 1;
			irq_prescaler = 341;

			if(data & 2) {
				// enabled
				irq_enabled = true;
				irq_counter = irq_reload;
			} else {
				// disabled
				irq_enabled = false;
			}

			// acknowledge
			// irq_pending = false;
			Irq = false;
			return true;
		case 0xF002: //$F002 (ack)
			Irq = false;
			// irq_pending = false;
			irq_enabled = irq_autoen;
			// SyncIRQ();
			return true;
	}
	return false;
}

uint32_t VRC6Mapper::mapaddr(uint16_t addr) {
	int lutidx = addr >> 10 | PPUBankingMode << 4;
	int bank = chr_banks_1k[Banks[lutidx]];
	if(chrA10replace) {
		bank &= Masks[lutidx];
		bank |= A10s[lutidx];
	}
	return (addr & 0x3ff) | (bank << 10);
}

uint8_t VRC6Mapper::ppuRead(uint16_t addr, bool readOnly) {
	if(addr < 0x2000 || NTROM) {
		return chr[mapaddr(addr) & chrMask];
	} else { // 0x2000 - 0x3EFF
		return vram[mapaddr(addr) & 0x7ff];
	}
}

void VRC6Mapper::ppuWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x2000 && !NTROM) {
		vram[mapaddr(addr) & 0x7ff] = data;
	}
}

void VRC6Mapper::HardReset() {
	Mapper::HardReset();
	prgBankOffset[0] = 0;
	prgBankOffset[1] = 0;
	prgBankOffset[2] = 0;
	prgBankOffset[3] = (prgBanks - 1) * 0x2000; // fixed last bank

    chr_banks_1k.fill(0);

	int idx = 0;
	for(int b003 = 0; b003 < 16; b003++) {
		for(int banknum = 0; banknum < 16; banknum++) {
			uint8_t bank = 0, mask = 0, a10 = 0;
			GetBankByte(b003, banknum, bank, mask, a10);
			Banks[idx] = bank;
			Masks[idx] = mask;
			A10s[idx] = a10;
			idx++;
		}
	}

    PPUBankingMode = 0;
	chrA10replace = false;
	NTROM = false;

	ramEnable = false;
	prgRam.fill(0);

	irq_enabled = false;
	irq_mode = false;
	irq_autoen = false;
	irq_reload = 0;
	irq_counter = 0;
	irq_prescaler = 0;
}

void VRC6Mapper::CpuClock() {
	if(!irq_enabled) return;

	if(irq_mode) { // cycle mode
		if(irq_counter == 0xFF) {
			Irq = true;
			irq_counter = irq_reload;
		} else
			irq_counter++;
	} else { // scanline mode
		irq_prescaler -= 3;
		if(irq_prescaler <= 0) {
			irq_prescaler += 341;
			if(irq_counter == 0xFF) {
				Irq = true;
				irq_counter = irq_reload;
			} else
				irq_counter++;
		}
	}
}

}
