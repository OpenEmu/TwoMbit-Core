
#define FLAG_C 0x1

typedef union {
	struct {
		u16 AF, BC, DE, HL, IX, IY, SP, PC;
	} w;
	
	struct {
		u8 F, A, C, B, E, D, L, H, IXl, IXh, IYl, IYh, SPl, SPh, PCl, PCh;
	} b;

	struct {
		bool c : 1; bool n : 1; bool pv : 1; bool unused_bit3 : 1;
		bool h : 1; bool unused_bit5 : 1; bool z : 1; bool s : 1;
	} bit;
} Regs;

Regs reg;
Regs reg_al;
u8 reg_r, reg_i;

u8* regPTR[14];
u16* regPTR16[7];

u8 data8;
u8 displacement;
u16 data16;

u8 mdr;
