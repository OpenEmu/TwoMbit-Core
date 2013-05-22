#include "vdp.h"

void Vdp::name_ram_load_mode0() {
	u8 line = vcounter % 8;
	tileDefinition = vram[ ((regs[4] & 7) << 11) + (tilePattern << 3) + line ];
}

void Vdp::color_address_select_mode0() {
	colAddress = (regs[3] << 6) + (tilePattern >> 3);
}
