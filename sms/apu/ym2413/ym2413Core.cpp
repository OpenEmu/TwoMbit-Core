#include "ym2413Core.h"
#include <math.h>
#include <string.h>

YM2413CORE::YM2413CORE() {
    initTlTable();
    initSinTable();
    OPLL_initalize();
}

void YM2413CORE::reset() {
    int c,s;
    int i;
    OPLL_initalize();

    ym2413.eg_timer = 0;
    ym2413.eg_cnt   = 0;
    ym2413.noise_rng = 1;
    ym2413.state = 0;

    for (i=0; i<19; i++) {
        for (c=0; c<8; c++) {
            ym2413.inst_tab[i][c] = table[i][c];
        }
    }

    writeReg(0x0f,0); /*test reg*/
    for(i = 0x3f ; i >= 0x10 ; i-- ) writeReg(i,0x00);

    for( c = 0 ; c < 9 ; c++ ) {
        YM2413_OPLL_CH* CH = &ym2413.P_CH[c];
        for(s = 0; s < 2; s++ ) {
            CH->SLOT[s].wavetable = 0;
            CH->SLOT[s].state     = EG_OFF;
            CH->SLOT[s].volume    = MAX_ATT_INDEX;
        }
    }
}

void YM2413CORE::OPLL_initalize(void) {
    memset(&ym2413,0,sizeof(Chip));
    int i;

    double freqbase = 1.0;

    for( i = 0 ; i < 1024; i++ ) {
        /* OPLL (YM2413) phase increment counter = 18bit */
        ym2413.fn_tab[i] = (u32)( (double)i * 64 * freqbase * (1<<(FREQ_SH-10)) ); /* -10 because chip works with 10.10 fixed point, while we use 16.16 */
    }

    ym2413.lfo_am_inc = (1.0 / 64.0 ) * (1<<LFO_SH) * freqbase;
    ym2413.lfo_pm_inc = (1.0 / 1024.0) * (1<<LFO_SH) * freqbase;
    ym2413.noise_f = (1.0 / 1.0) * (1<<FREQ_SH) * freqbase;
    ym2413.eg_timer_add  = (1<<EG_SH) * freqbase;
    ym2413.eg_timer_overflow = ( 1 ) * (1<<EG_SH);
}

int YM2413CORE::YM2413Update()
{
    int out;
    output[0] = 0;
    output[1] = 0;
    advance_lfo();
    chan_calc(&ym2413.P_CH[0]);
    chan_calc(&ym2413.P_CH[1]);
    chan_calc(&ym2413.P_CH[2]);
    chan_calc(&ym2413.P_CH[3]);
    chan_calc(&ym2413.P_CH[4]);
    chan_calc(&ym2413.P_CH[5]);

    if(!(ym2413.rhythm&0x20)) {
        chan_calc(&ym2413.P_CH[6]);
        chan_calc(&ym2413.P_CH[7]);
        chan_calc(&ym2413.P_CH[8]);
    } else {  /* Rhythm part */
        rhythm_calc(&ym2413.P_CH[0], (ym2413.noise_rng>>0)&1 );
    }
    /* Melody (MO) & Rythm (RO) outputs mixing & amplification (latched bit controls FM output) */
    out = (output[0] + (output[1] * 2)) * 2 * ym2413.state;
    //*buffer = out;
    advance();
    return out;
}

signed int YM2413CORE::op_calc(u32 phase, unsigned int env, signed int pm, unsigned int wave_tab, bool pmShift) {
    u32 p = (env<<5) + sin_tab[wave_tab + ((((signed int)((phase & ~FREQ_MASK) + (pmShift ? pm<<17 : pm))) >> FREQ_SH ) & SIN_MASK) ];

    if (p >= TL_TAB_LEN) return 0;
    return tl_tab[p];
}

#define volume_calc(OP) ((OP)->TLL + ((u32)(OP)->volume) + (LFO_AM & (OP)->AMmask))

/* calculate output */
void YM2413CORE::chan_calc( YM2413_OPLL_CH *CH ) {
    YM2413_OPLL_SLOT *SLOT;
    unsigned int env;
    signed int out;
    signed int phase_modulation;  /* phase modulation input (SLOT 2) */

    /* SLOT 1 */
    SLOT = &CH->SLOT[SLOT1];
    env  = volume_calc(SLOT);
    out  = SLOT->op1_out[0] + SLOT->op1_out[1];

    SLOT->op1_out[0] = SLOT->op1_out[1];
    phase_modulation = SLOT->op1_out[0];

    SLOT->op1_out[1] = 0;

    if( env < ENV_QUIET ) {
        if (!SLOT->fb_shift) out = 0;
        SLOT->op1_out[1] = op_calc(SLOT->phase, env, (out<<SLOT->fb_shift), SLOT->wavetable, false );
    }

    /* SLOT 2 */
    SLOT++;
    env = volume_calc(SLOT);
    if( env < ENV_QUIET ) {
        output[0] += op_calc(SLOT->phase, env, phase_modulation, SLOT->wavetable, true);
    }
}

void YM2413CORE::rhythm_calc( YM2413_OPLL_CH *CH, unsigned int noise ) {
    YM2413_OPLL_SLOT *SLOT;
    signed int out;
    unsigned int env;
    signed int phase_modulation;  /* phase modulation input (SLOT 2) */

    /* SLOT 1 */
    SLOT = &CH[6].SLOT[SLOT1];
    env = volume_calc(SLOT);

    out = SLOT->op1_out[0] + SLOT->op1_out[1];
    SLOT->op1_out[0] = SLOT->op1_out[1];

    phase_modulation = SLOT->op1_out[0];

    SLOT->op1_out[1] = 0;
    if( env < ENV_QUIET ) {
        if (!SLOT->fb_shift) out = 0;
        SLOT->op1_out[1] = op_calc(SLOT->phase, env, (out<<SLOT->fb_shift), SLOT->wavetable,false );
    }

    /* SLOT 2 */
    SLOT++;
    env = volume_calc(SLOT);
    if( env < ENV_QUIET )
        output[1] += op_calc(SLOT->phase, env, phase_modulation, SLOT->wavetable, true);

    /* High Hat (verified on real YM3812) */
    env = volume_calc(&CH[7].SLOT[SLOT1]);
    if( env < ENV_QUIET ) {
        /* base frequency derived from operator 1 in channel 7 */
        unsigned char bit7 = ((CH[7].SLOT[SLOT1].phase>>FREQ_SH)>>7)&1;
        unsigned char bit3 = ((CH[7].SLOT[SLOT1].phase>>FREQ_SH)>>3)&1;
        unsigned char bit2 = ((CH[7].SLOT[SLOT1].phase>>FREQ_SH)>>2)&1;

        unsigned char res1 = (bit2 ^ bit7) | bit3;

        u32 phase = res1 ? (0x200|(0xd0>>2)) : 0xd0;

        /* enable gate based on frequency of operator 2 in channel 8 */
        unsigned char bit5e= ((CH[8].SLOT[SLOT2].phase>>FREQ_SH)>>5)&1;
        unsigned char bit3e= ((CH[8].SLOT[SLOT2].phase>>FREQ_SH)>>3)&1;

        unsigned char res2 = (bit3e | bit5e);

        /* when res2 = 0 pass the phase from calculation above (res1); */
        /* when res2 = 1 phase = 0x200 | (0xd0>>2); */
        if (res2)
          phase = (0x200|(0xd0>>2));

        if (phase&0x200) {
            if (noise)
                phase = 0x200|0xd0;
        } else {
            if (noise)
                phase = 0xd0>>2;
        }

        output[1] += op_calc(phase<<FREQ_SH, env, 0, CH[7].SLOT[SLOT1].wavetable, true);
    }

    env = volume_calc(&CH[7].SLOT[SLOT2]);
    if( env < ENV_QUIET ) {
        unsigned char bit8 = ((CH[7].SLOT[SLOT1].phase>>FREQ_SH)>>8)&1;

        u32 phase = bit8 ? 0x200 : 0x100;

        if (noise)
            phase ^= 0x100;

        output[1] += op_calc(phase<<FREQ_SH, env, 0, CH[7].SLOT[SLOT2].wavetable, true);
    }

    /* Tom Tom (verified on real YM3812) */
    env = volume_calc(&CH[8].SLOT[SLOT1]);
    if( env < ENV_QUIET )
        output[1] += op_calc(CH[8].SLOT[SLOT1].phase, env, 0, CH[8].SLOT[SLOT1].wavetable,true);

    /* Top Cymbal (verified on real YM2413) */
    env = volume_calc(&CH[8].SLOT[SLOT2]);
    if( env < ENV_QUIET ) {
        /* base frequency derived from operator 1 in channel 7 */
        unsigned char bit7 = ((CH[7].SLOT[SLOT1].phase>>FREQ_SH)>>7)&1;
        unsigned char bit3 = ((CH[7].SLOT[SLOT1].phase>>FREQ_SH)>>3)&1;
        unsigned char bit2 = ((CH[7].SLOT[SLOT1].phase>>FREQ_SH)>>2)&1;

        unsigned char res1 = (bit2 ^ bit7) | bit3;

        u32 phase = res1 ? 0x300 : 0x100;

        /* enable gate based on frequency of operator 2 in channel 8 */
        unsigned char bit5e= ((CH[8].SLOT[SLOT2].phase>>FREQ_SH)>>5)&1;
        unsigned char bit3e= ((CH[8].SLOT[SLOT2].phase>>FREQ_SH)>>3)&1;

        unsigned char res2 = (bit3e | bit5e);
        if (res2)
          phase = 0x300;

        output[1] += op_calc(phase<<FREQ_SH, env, 0, CH[8].SLOT[SLOT2].wavetable,true);
    }
}


/* advance LFO to next sample */
void YM2413CORE::advance_lfo(void) {
    /* LFO */
    ym2413.lfo_am_cnt += ym2413.lfo_am_inc;
    if (ym2413.lfo_am_cnt >= (LFO_AM_TAB_ELEMENTS<<LFO_SH) )  /* lfo_am_table is 210 elements long */
    ym2413.lfo_am_cnt -= (LFO_AM_TAB_ELEMENTS<<LFO_SH);

    LFO_AM = lfo_am_table[ ym2413.lfo_am_cnt >> LFO_SH ] >> 1;

    ym2413.lfo_pm_cnt += ym2413.lfo_pm_inc;
    LFO_PM = (ym2413.lfo_pm_cnt>>LFO_SH) & 7;
}

/* advance to next sample */
void YM2413CORE::advance(void) {
    YM2413_OPLL_CH *CH;
    YM2413_OPLL_SLOT *op;
    unsigned int i;

    /* Envelope Generator */
    ym2413.eg_timer += ym2413.eg_timer_add;

    while (ym2413.eg_timer >= ym2413.eg_timer_overflow) {
        ym2413.eg_timer -= ym2413.eg_timer_overflow;

        ym2413.eg_cnt++;

        for (i=0; i<9*2; i++) {
            CH  = &ym2413.P_CH[i>>1];

            op  = &CH->SLOT[i&1];

            switch(op->state)
            {
            case EG_DMP:    /* dump phase */
                if ( !(ym2413.eg_cnt & ((1<<op->eg_sh_dp)-1) ) )
                {
                    op->volume += eg_inc[op->eg_sel_dp + ((ym2413.eg_cnt>>op->eg_sh_dp)&7)];

                    if ( op->volume >= MAX_ATT_INDEX )
                    {
                        op->volume = MAX_ATT_INDEX;
                        op->state = EG_ATT;
                        /* restart Phase Generator  */
                        op->phase = 0;
                    }
                }
                break;

            case EG_ATT:    /* attack phase */
                if ( !(ym2413.eg_cnt & ((1<<op->eg_sh_ar)-1) ) )
                {
                    op->volume += (~op->volume *
                                   (eg_inc[op->eg_sel_ar + ((ym2413.eg_cnt>>op->eg_sh_ar)&7)])
                                   ) >>2;

                    if (op->volume <= MIN_ATT_INDEX)
                    {
                        op->volume = MIN_ATT_INDEX;
                        op->state = EG_DEC;
                    }
                }
                break;

            case EG_DEC:  /* decay phase */
                if ( !(ym2413.eg_cnt & ((1<<op->eg_sh_dr)-1) ) )
                {
                    op->volume += eg_inc[op->eg_sel_dr + ((ym2413.eg_cnt>>op->eg_sh_dr)&7)];

                    if ( op->volume >= op->sl )
                        op->state = EG_SUS;
                }
                break;

            case EG_SUS:  /* sustain phase */
                if(op->eg_type);    /* non-percussive mode (sustained tone) */
                else        /* percussive mode */
                {
                    /* during sustain phase chip adds Release Rate (in percussive mode) */
                    if ( !(ym2413.eg_cnt & ((1<<op->eg_sh_rr)-1) ) )
                    {
                        op->volume += eg_inc[op->eg_sel_rr + ((ym2413.eg_cnt>>op->eg_sh_rr)&7)];

                        if ( op->volume >= MAX_ATT_INDEX )
                            op->volume = MAX_ATT_INDEX;
                    }
                }
                break;
            case EG_REL:  /* release phase */
                if ( (i&1) || ((ym2413.rhythm&0x20) && (i>=12)) )/* exclude modulators */
                {
                    if(op->eg_type)    /* non-percussive mode (sustained tone) */
                    {
                        if (CH->sus)
                        {
                            if ( !(ym2413.eg_cnt & ((1<<op->eg_sh_rs)-1) ) )
                            {
                                op->volume += eg_inc[op->eg_sel_rs + ((ym2413.eg_cnt>>op->eg_sh_rs)&7)];
                                if ( op->volume >= MAX_ATT_INDEX )
                                {
                                    op->volume = MAX_ATT_INDEX;
                                    op->state = EG_OFF;
                                }
                            }
                        }
                        else
                        {
                            if ( !(ym2413.eg_cnt & ((1<<op->eg_sh_rr)-1) ) )
                            {
                                op->volume += eg_inc[op->eg_sel_rr + ((ym2413.eg_cnt>>op->eg_sh_rr)&7)];
                                if ( op->volume >= MAX_ATT_INDEX )
                                {
                                    op->volume = MAX_ATT_INDEX;
                                    op->state = EG_OFF;
                                }
                            }
                        }
                    }
                    else        /* percussive mode */
                    {
                        if ( !(ym2413.eg_cnt & ((1<<op->eg_sh_rs)-1) ) )
                        {
                            op->volume += eg_inc[op->eg_sel_rs + ((ym2413.eg_cnt>>op->eg_sh_rs)&7)];
                            if ( op->volume >= MAX_ATT_INDEX )
                            {
                                op->volume = MAX_ATT_INDEX;
                                op->state = EG_OFF;
                            }
                        }
                    }
                }
                break;

            default:
                break;
            }
        }
    }

    for (i=0; i<9*2; i++)
    {
        CH  = &ym2413.P_CH[i/2];
        op  = &CH->SLOT[i&1];

        /* Phase Generator */
        if(op->vib)
        {
            u8 block;

            unsigned int fnum_lfo   = 8*((CH->block_fnum&0x01c0) >> 6);
            unsigned int block_fnum = CH->block_fnum * 2;
            signed int lfo_fn_table_index_offset = lfo_pm_table[LFO_PM + fnum_lfo ];

            if (lfo_fn_table_index_offset)  /* LFO phase modulation active */
            {
                block_fnum += lfo_fn_table_index_offset;
                block = (block_fnum&0x1c00) >> 10;
                op->phase += (ym2413.fn_tab[block_fnum&0x03ff] >> (7-block)) * op->mul;
            }
            else  /* LFO phase modulation  = zero */
            {
                op->phase += op->freq;
            }
        }
        else  /* LFO phase modulation disabled for this operator */
        {
            op->phase += op->freq;
        }
    }

    ym2413.noise_p += ym2413.noise_f;
    i = ym2413.noise_p >> FREQ_SH;    /* number of events (shifts of the shift register) */
    ym2413.noise_p &= FREQ_MASK;
    while (i) {
        if (ym2413.noise_rng & 1) ym2413.noise_rng ^= 0x800302;
        ym2413.noise_rng >>= 1;
        i--;
    }
}
