
#include "ym2413Core.h"
#include "serializer.h"

void YM2413CORE::coreSerialize(Serializer& s) {
    for (unsigned i = 0; i < 9; i++) {
        for (unsigned j = 0; j < 2; j++) {
            s.integer(ym2413.P_CH[i].SLOT[j].ar);
            s.integer(ym2413.P_CH[i].SLOT[j].dr);
            s.integer(ym2413.P_CH[i].SLOT[j].rr);
            s.integer(ym2413.P_CH[i].SLOT[j].KSR);
            s.integer(ym2413.P_CH[i].SLOT[j].ksl);
            s.integer(ym2413.P_CH[i].SLOT[j].ksr);
            s.integer(ym2413.P_CH[i].SLOT[j].mul);
            s.integer(ym2413.P_CH[i].SLOT[j].phase);
            s.integer(ym2413.P_CH[i].SLOT[j].freq);
            s.integer(ym2413.P_CH[i].SLOT[j].fb_shift);
            s.integer(ym2413.P_CH[i].SLOT[j].op1_out[0]);
            s.integer(ym2413.P_CH[i].SLOT[j].op1_out[1]);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_type);
            s.integer(ym2413.P_CH[i].SLOT[j].state);
            s.integer(ym2413.P_CH[i].SLOT[j].TL);
            s.integer(ym2413.P_CH[i].SLOT[j].TLL);
            s.integer(ym2413.P_CH[i].SLOT[j].volume);
            s.integer(ym2413.P_CH[i].SLOT[j].sl);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sh_dp);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sel_dp);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sh_ar);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sel_ar);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sh_dr);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sel_dr);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sh_rr);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sel_rr);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sh_rs);
            s.integer(ym2413.P_CH[i].SLOT[j].eg_sel_rs);
            s.integer(ym2413.P_CH[i].SLOT[j].key);
            s.integer(ym2413.P_CH[i].SLOT[j].AMmask);
            s.integer(ym2413.P_CH[i].SLOT[j].vib);
            s.integer(ym2413.P_CH[i].SLOT[j].wavetable);
        }
        s.integer(ym2413.P_CH[i].block_fnum);
        s.integer(ym2413.P_CH[i].fc);
        s.integer(ym2413.P_CH[i].ksl_base);
        s.integer(ym2413.P_CH[i].kcode);
        s.integer(ym2413.P_CH[i].sus);
    }
    for (unsigned i = 0; i < 9; i++) {
        s.integer(ym2413.instvol_r[i]);
    }

    s.integer(ym2413.eg_cnt);
    s.integer(ym2413.eg_timer);
    s.integer(ym2413.eg_timer_add);
    s.integer(ym2413.eg_timer_overflow);
    s.integer(ym2413.rhythm);
    s.integer(ym2413.lfo_am_cnt);
    s.integer(ym2413.lfo_am_inc);
    s.integer(ym2413.lfo_pm_cnt);
    s.integer(ym2413.lfo_pm_inc);
    s.integer(ym2413.noise_rng);
    s.integer(ym2413.noise_p);
    s.integer(ym2413.noise_f);

    for (unsigned i = 0; i < 19; i++) {
        for (unsigned j = 0; j < 8; j++) {
            s.integer(ym2413.inst_tab[i][j]);
        }
    }
    for (unsigned i = 0; i < 1024; i++) {
        s.integer(ym2413.fn_tab[i]);
    }
    s.integer(ym2413.state);

    s.integer(output[0]);
    s.integer(output[1]);
    s.integer(LFO_AM);
    s.integer(LFO_PM);
}
