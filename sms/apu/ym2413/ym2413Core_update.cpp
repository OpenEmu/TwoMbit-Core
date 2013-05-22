
#include "ym2413Core.h"
#include <math.h>

/* update phase increment counter of operator (also update the EG rates if necessary) */
void YM2413CORE::CALC_FCSLOT(YM2413CORE::YM2413_OPLL_CH* CH, YM2413CORE::YM2413_OPLL_SLOT* SLOT) {
    int ksr;
    u32 SLOT_rs;
    u32 SLOT_dp;

    /* (frequency) phase increment counter */
    SLOT->freq = CH->fc * SLOT->mul;
    ksr = CH->kcode >> SLOT->KSR;

    if( SLOT->ksr != ksr ) {
        SLOT->ksr = ksr;

        /* calculate envelope generator rates */
        if ((SLOT->ar + SLOT->ksr) < 16+62) {
            SLOT->eg_sh_ar  = eg_rate_shift [SLOT->ar + SLOT->ksr ];
            SLOT->eg_sel_ar = eg_rate_select[SLOT->ar + SLOT->ksr ];
        } else {
            SLOT->eg_sh_ar  = 0;
            SLOT->eg_sel_ar = 13*RATE_STEPS;
        }
        SLOT->eg_sh_dr  = eg_rate_shift [SLOT->dr + SLOT->ksr ];
        SLOT->eg_sel_dr = eg_rate_select[SLOT->dr + SLOT->ksr ];
        SLOT->eg_sh_rr  = eg_rate_shift [SLOT->rr + SLOT->ksr ];
        SLOT->eg_sel_rr = eg_rate_select[SLOT->rr + SLOT->ksr ];
    }

    if (CH->sus)    SLOT_rs  = 16 + (5<<2);
    else            SLOT_rs  = 16 + (7<<2);

    SLOT->eg_sh_rs  = eg_rate_shift [SLOT_rs + SLOT->ksr ];
    SLOT->eg_sel_rs = eg_rate_select[SLOT_rs + SLOT->ksr ];

    SLOT_dp = 16 + (13<<2);
    SLOT->eg_sh_dp  = eg_rate_shift [SLOT_dp + SLOT->ksr ];
    SLOT->eg_sel_dp = eg_rate_select[SLOT_dp + SLOT->ksr ];
}

void YM2413CORE::set_mul(int slot,int v) {
  YM2413_OPLL_CH   *CH   = &ym2413.P_CH[slot/2];
  YM2413_OPLL_SLOT *SLOT = &CH->SLOT[slot&1];

  SLOT->mul     = getMul(v&0x0f);
  SLOT->KSR     = (v&0x10) ? 0 : 2;
  SLOT->eg_type = (v&0x20);
  SLOT->vib     = (v&0x40);
  SLOT->AMmask  = (v&0x80) ? ~0 : 0;
  CALC_FCSLOT(CH, SLOT);
}

void YM2413CORE::set_ksl_tl(int chan,int v) {
    YM2413_OPLL_CH   *CH   = &ym2413.P_CH[chan];
    /* modulator */
    YM2413_OPLL_SLOT *SLOT = &CH->SLOT[SLOT1];

    int ksl = v>>6; /* 0 / 1.5 / 3.0 / 6.0 dB/OCT */

    SLOT->ksl = ksl ? 3-ksl : 31;
    SLOT->TL  = (v&0x3f)<<(ENV_BITS-2-7); /* 7 bits TL (bit 6 = always 0) */
    SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);
}

/* set attack rate & decay rate  */
void YM2413CORE::set_ar_dr(int slot, int v) {
    YM2413_OPLL_CH   *CH   = &ym2413.P_CH[slot/2];
    YM2413_OPLL_SLOT *SLOT = &CH->SLOT[slot&1];

    SLOT->ar = (v>>4) ? 16 + ((v>>4)  <<2) : 0;

    if ((SLOT->ar + SLOT->ksr) < 16+62) {
        SLOT->eg_sh_ar  = eg_rate_shift [SLOT->ar + SLOT->ksr ];
        SLOT->eg_sel_ar = eg_rate_select[SLOT->ar + SLOT->ksr ];
    } else {
        SLOT->eg_sh_ar  = 0;
        SLOT->eg_sel_ar = 13*RATE_STEPS;
    }

    SLOT->dr    = (v&0x0f)? 16 + ((v&0x0f)<<2) : 0;
    SLOT->eg_sh_dr  = eg_rate_shift [SLOT->dr + SLOT->ksr ];
    SLOT->eg_sel_dr = eg_rate_select[SLOT->dr + SLOT->ksr ];
}

/* set sustain level & release rate */
void YM2413CORE::set_sl_rr(int slot,int v) {
    YM2413_OPLL_CH   *CH   = &ym2413.P_CH[slot/2];
    YM2413_OPLL_SLOT *SLOT = &CH->SLOT[slot&1];

    SLOT->sl  = sl_tab[ v>>4 ];

    SLOT->rr  = (v&0x0f)? 16 + ((v&0x0f)<<2) : 0;
    SLOT->eg_sh_rr  = eg_rate_shift [SLOT->rr + SLOT->ksr ];
    SLOT->eg_sel_rr = eg_rate_select[SLOT->rr + SLOT->ksr ];
}

/* set ksl , waveforms, feedback */
void YM2413CORE::set_ksl_wave_fb(int chan,int v) {
    YM2413_OPLL_CH   *CH   = &ym2413.P_CH[chan];
    /* modulator */
    YM2413_OPLL_SLOT *SLOT = &CH->SLOT[SLOT1];
    SLOT->wavetable = ((v&0x08)>>3)*SIN_LEN;
    SLOT->fb_shift  = (v&7) ? (v&7) + 8 : 0;

    /*carrier*/
    SLOT = &CH->SLOT[SLOT2];
    int ksl = v>>6; /* 0 / 1.5 / 3.0 / 6.0 dB/OCT */

    SLOT->ksl = ksl ? 3-ksl : 31;
    SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);

    SLOT->wavetable = ((v&0x10)>>4)*SIN_LEN;
}

void YM2413CORE::update_instrument_zero(u8 r) {
    u8* inst = &ym2413.inst_tab[0][0]; /* point to user instrument */
    u32 chan;

    u32 chan_max = 9;
    if (ym2413.rhythm & 0x20) chan_max=6;

    switch(r&7) {
        case 0:
            for (chan=0; chan < chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_mul(chan*2, inst[0]);
                }
            } break;
        case 1:
            for (chan=0; chan<chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_mul(chan*2+1, inst[1]);
                }
            } break;
        case 2:
            for (chan=0; chan<chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_ksl_tl(chan, inst[2]);
                }
            } break;
        case 3:
            for (chan=0; chan<chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_ksl_wave_fb(chan, inst[3]);
                }
            } break;
        case 4:
            for (chan=0; chan<chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_ar_dr(chan*2, inst[4]);
                }
            } break;
        case 5:
            for (chan=0; chan<chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_ar_dr(chan*2+1, inst[5]);
                }
            } break;
        case 6:
            for (chan=0; chan<chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_sl_rr(chan*2, inst[6]);
                }
            } break;
        case 7:
            for (chan=0; chan<chan_max; chan++) {
                if ((ym2413.instvol_r[chan]&0xf0)==0) {
                    set_sl_rr(chan*2+1, inst[7]);
                }
            } break;
    }
}

void YM2413CORE::load_instrument(u32 chan, u32 slot, u8* inst ) {
    set_mul(slot, inst[0]);
    set_mul(slot+1, inst[1]);
    set_ksl_tl(chan, inst[2]);
    set_ksl_wave_fb(chan, inst[3]);
    set_ar_dr(slot,   inst[4]);
    set_ar_dr(slot+1, inst[5]);
    set_sl_rr(slot,   inst[6]);
    set_sl_rr(slot+1, inst[7]);
}

/* write a value v to register r on chip chip */
void YM2413CORE::writeReg(int r, int v) {
    YM2413_OPLL_CH* CH;
    YM2413_OPLL_SLOT* SLOT;

    /* adjust bus to 8 bits */
    r &= 0xff;
    v &= 0xff;

    switch(r&0xf0) {
    case 0x00: { /* 00-0f:control */
        switch(r&0x0f) {
            case 0x00:  /* AM/VIB/EGTYP/KSR/MULTI (modulator) */
            case 0x01:  /* AM/VIB/EGTYP/KSR/MULTI (carrier) */
            case 0x02:  /* Key Scale Level, Total Level (modulator) */
            case 0x03:  /* Key Scale Level, carrier waveform, modulator waveform, Feedback */
            case 0x04:  /* Attack, Decay (modulator) */
            case 0x05:  /* Attack, Decay (carrier) */
            case 0x06:  /* Sustain, Release (modulator) */
            case 0x07: {  /* Sustain, Release (carrier) */
                ym2413.inst_tab[0][r] = v;
                update_instrument_zero(r);
                break;
            }
            case 0x0e: {  /* x, x, r,bd,sd,tom,tc,hh */
                if(v&0x20) {
                /* rhythm OFF to ON */
                    if ((ym2413.rhythm&0x20)==0) {
                        /* Load instrument settings for channel seven(chan=6 since we're zero based). (Bass drum) */
                        load_instrument(6, 12, &ym2413.inst_tab[16][0]);
                        /* Load instrument settings for channel eight. (High hat and snare drum) */
                        load_instrument(7, 14, &ym2413.inst_tab[17][0]);

                        CH   = &ym2413.P_CH[7];
                        SLOT = &CH->SLOT[SLOT1]; /* modulator envelope is HH */
                        SLOT->TL  = ((ym2413.instvol_r[7]>>4)<<2)<<(ENV_BITS-2-7); /* 7 bits TL (bit 6 = always 0) */
                        SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);

                        /* Load instrument settings for channel nine. (Tom-tom and top cymbal) */
                        load_instrument(8, 16, &ym2413.inst_tab[18][0]);

                        CH   = &ym2413.P_CH[8];
                        SLOT = &CH->SLOT[SLOT1]; /* modulator envelope is TOM */
                        SLOT->TL  = ((ym2413.instvol_r[8]>>4)<<2)<<(ENV_BITS-2-7); /* 7 bits TL (bit 6 = always 0) */
                        SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);
                    }
                    /* BD key on/off */
                    if(v&0x10) {
                        KEY_ON (&ym2413.P_CH[6].SLOT[SLOT1], 2);
                        KEY_ON (&ym2413.P_CH[6].SLOT[SLOT2], 2);
                    } else {
                        KEY_OFF(&ym2413.P_CH[6].SLOT[SLOT1],~2);
                        KEY_OFF(&ym2413.P_CH[6].SLOT[SLOT2],~2);
                    }

                    /* HH key on/off */
                    if(v&0x01) KEY_ON (&ym2413.P_CH[7].SLOT[SLOT1], 2);
                    else       KEY_OFF(&ym2413.P_CH[7].SLOT[SLOT1],~2);

                    /* SD key on/off */
                    if(v&0x08) KEY_ON (&ym2413.P_CH[7].SLOT[SLOT2], 2);
                    else       KEY_OFF(&ym2413.P_CH[7].SLOT[SLOT2],~2);

                    /* TOM key on/off */
                    if(v&0x04) KEY_ON (&ym2413.P_CH[8].SLOT[SLOT1], 2);
                    else       KEY_OFF(&ym2413.P_CH[8].SLOT[SLOT1],~2);

                    /* TOP-CY key on/off */
                    if(v&0x02) KEY_ON (&ym2413.P_CH[8].SLOT[SLOT2], 2);
                    else       KEY_OFF(&ym2413.P_CH[8].SLOT[SLOT2],~2);
                } else {
                    /* rhythm ON to OFF */
                    if (ym2413.rhythm&0x20) {
                        /* Load instrument settings for channel seven(chan=6 since we're zero based).*/
                        load_instrument(6, 12, &ym2413.inst_tab[ym2413.instvol_r[6]>>4][0]);
                        /* Load instrument settings for channel eight.*/
                        load_instrument(7, 14, &ym2413.inst_tab[ym2413.instvol_r[7]>>4][0]);
                        /* Load instrument settings for channel nine.*/
                        load_instrument(8, 16, &ym2413.inst_tab[ym2413.instvol_r[8]>>4][0]);
                    }
                    /* BD key off */
                    KEY_OFF(&ym2413.P_CH[6].SLOT[SLOT1],~2);
                    KEY_OFF(&ym2413.P_CH[6].SLOT[SLOT2],~2);

                    /* HH key off */
                    KEY_OFF(&ym2413.P_CH[7].SLOT[SLOT1],~2);

                    /* SD key off */
                    KEY_OFF(&ym2413.P_CH[7].SLOT[SLOT2],~2);

                    /* TOM key off */
                    KEY_OFF(&ym2413.P_CH[8].SLOT[SLOT1],~2);

                    /* TOP-CY off */
                    KEY_OFF(&ym2413.P_CH[8].SLOT[SLOT2],~2);
                }
                ym2413.rhythm = v&0x3f;
                break;
            }
        } break;
    }
    case 0x10:
    case 0x20: {
        int block_fnum;
        int chan = r&0x0f;

        if (chan >= 9)
            chan -= 9;  /* verified on real YM2413 */

        CH = &ym2413.P_CH[chan];

        if(r&0x10) {
            /* 10-18: FNUM 0-7 */
            block_fnum = (CH->block_fnum&0x0f00) | v;
        } else {
            /* 20-28: suson, keyon, block, FNUM 8 */
            block_fnum = ((v&0x0f)<<8) | (CH->block_fnum&0xff);

            if(v&0x10) {
                KEY_ON (&CH->SLOT[SLOT1], 1);
                KEY_ON (&CH->SLOT[SLOT2], 1);
            } else {
                KEY_OFF(&CH->SLOT[SLOT1],~1);
                KEY_OFF(&CH->SLOT[SLOT2],~1);
            }
            CH->sus = v & 0x20;
        }

        /* update */
        if(CH->block_fnum != block_fnum) {
            CH->block_fnum = block_fnum;

            /* BLK 2,1,0 bits -> bits 3,2,1 of kcode, FNUM MSB -> kcode LSB */
            CH->kcode    = (block_fnum&0x0f00)>>8;

            CH->ksl_base = getKsl(block_fnum>>5);

            block_fnum   = block_fnum * 2;
            u8 block  = (block_fnum&0x1c00) >> 10;
            CH->fc    = ym2413.fn_tab[block_fnum&0x03ff] >> (7-block);

            /* refresh Total Level in both SLOTs of this channel */
            CH->SLOT[SLOT1].TLL = CH->SLOT[SLOT1].TL + (CH->ksl_base>>CH->SLOT[SLOT1].ksl);
            CH->SLOT[SLOT2].TLL = CH->SLOT[SLOT2].TL + (CH->ksl_base>>CH->SLOT[SLOT2].ksl);

            /* refresh frequency counter in both SLOTs of this channel */
            CALC_FCSLOT(CH,&CH->SLOT[SLOT1]);
            CALC_FCSLOT(CH,&CH->SLOT[SLOT2]);
        }
        break;
    }

    case 0x30: { /* inst 4 MSBs, VOL 4 LSBs */
        int chan = r&0x0f;

        if (chan >= 9)
            chan -= 9;  /* verified on real YM2413 */

        CH   = &ym2413.P_CH[chan];
        SLOT = &CH->SLOT[SLOT2]; /* carrier */
        SLOT->TL  = ((v&0x0f)<<2)<<(ENV_BITS-2-7); /* 7 bits TL (bit 6 = always 0) */
        SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);

        /*check wether we are in rhythm mode and handle instrument/volume register accordingly*/
        if ((chan>=6) && (ym2413.rhythm&0x20)) {
            /* we're in rhythm mode*/
            if (chan>=7) { /* only for channel 7 and 8 (channel 6 is handled in usual way)*/
                SLOT = &CH->SLOT[SLOT1]; /* modulator envelope is HH(chan=7) or TOM(chan=8) */
                SLOT->TL  = ((v>>4)<<2)<<(ENV_BITS-2-7); /* 7 bits TL (bit 6 = always 0) */
                SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);
            }
        } else {
            if ((ym2413.instvol_r[chan]&0xf0) != (v&0xf0)) {
                ym2413.instvol_r[chan] = v;  /* store for later use */
                load_instrument(chan, chan * 2, &ym2413.inst_tab[v>>4][0]);
            }
        }
    } break;
    default: break;
    }
}

void YM2413CORE::KEY_ON(YM2413_OPLL_SLOT* SLOT, u32 key_set) {
    if( !SLOT->key ) {
        SLOT->state = EG_DMP;
    }
    SLOT->key |= key_set;
}

void YM2413CORE::KEY_OFF(YM2413_OPLL_SLOT *SLOT, u32 key_clr) {
    if( SLOT->key ) {
        SLOT->key &= key_clr;

        if( !SLOT->key ) {
            /* phase -> Release */
            if (SLOT->state > EG_REL) SLOT->state = EG_REL;
        }
    }
}
