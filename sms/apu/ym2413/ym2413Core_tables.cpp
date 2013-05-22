#include "ym2413Core.h"
#include <math.h>

#define DV (0.1875/1.0)
const double YM2413CORE::ksl_tab[8*16] = {
    /* OCT 0 */
    0.000/DV, 0.000/DV, 0.000/DV, 0.000/DV,
    0.000/DV, 0.000/DV, 0.000/DV, 0.000/DV,
    0.000/DV, 0.000/DV, 0.000/DV, 0.000/DV,
    0.000/DV, 0.000/DV, 0.000/DV, 0.000/DV,
    /* OCT 1 */
    0.000/DV, 0.000/DV, 0.000/DV, 0.000/DV,
    0.000/DV, 0.000/DV, 0.000/DV, 0.000/DV,
    0.000/DV, 0.750/DV, 1.125/DV, 1.500/DV,
    1.875/DV, 2.250/DV, 2.625/DV, 3.000/DV,
    /* OCT 2 */
    0.000/DV, 0.000/DV, 0.000/DV, 0.000/DV,
    0.000/DV, 1.125/DV, 1.875/DV, 2.625/DV,
    3.000/DV, 3.750/DV, 4.125/DV, 4.500/DV,
    4.875/DV, 5.250/DV, 5.625/DV, 6.000/DV,
    /* OCT 3 */
    0.000/DV, 0.000/DV, 0.000/DV, 1.875/DV,
    3.000/DV, 4.125/DV, 4.875/DV, 5.625/DV,
    6.000/DV, 6.750/DV, 7.125/DV, 7.500/DV,
    7.875/DV, 8.250/DV, 8.625/DV, 9.000/DV,
    /* OCT 4 */
    0.000/DV, 0.000/DV, 3.000/DV, 4.875/DV,
    6.000/DV, 7.125/DV, 7.875/DV, 8.625/DV,
    9.000/DV, 9.750/DV,10.125/DV,10.500/DV,
    10.875/DV,11.250/DV,11.625/DV,12.000/DV,
    /* OCT 5 */
    0.000/DV, 3.000/DV, 6.000/DV, 7.875/DV,
    9.000/DV,10.125/DV,10.875/DV,11.625/DV,
    12.000/DV,12.750/DV,13.125/DV,13.500/DV,
    13.875/DV,14.250/DV,14.625/DV,15.000/DV,
    /* OCT 6 */
    0.000/DV, 6.000/DV, 9.000/DV,10.875/DV,
    12.000/DV,13.125/DV,13.875/DV,14.625/DV,
    15.000/DV,15.750/DV,16.125/DV,16.500/DV,
    16.875/DV,17.250/DV,17.625/DV,18.000/DV,
    /* OCT 7 */
    0.000/DV, 9.000/DV,12.000/DV,13.875/DV,
    15.000/DV,16.125/DV,16.875/DV,17.625/DV,
    18.000/DV,18.750/DV,19.125/DV,19.500/DV,
    19.875/DV,20.250/DV,20.625/DV,21.000/DV
};
#undef DV

#define O(a) (a*1)
const unsigned char YM2413CORE::eg_rate_shift[16+64+16]={ /* Envelope Generator counter shifts (16 + 64 rates + 16 RKS) */
    /* 16 infinite time rates */
    O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
    O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),

    /* rates 00-12 */
    O(13),O(13),O(13),O(13),
    O(12),O(12),O(12),O(12),
    O(11),O(11),O(11),O(11),
    O(10),O(10),O(10),O(10),
    O( 9),O( 9),O( 9),O( 9),
    O( 8),O( 8),O( 8),O( 8),
    O( 7),O( 7),O( 7),O( 7),
    O( 6),O( 6),O( 6),O( 6),
    O( 5),O( 5),O( 5),O( 5),
    O( 4),O( 4),O( 4),O( 4),
    O( 3),O( 3),O( 3),O( 3),
    O( 2),O( 2),O( 2),O( 2),
    O( 1),O( 1),O( 1),O( 1),

    /* rate 13 */
    O( 0),O( 0),O( 0),O( 0),

    /* rate 14 */
    O( 0),O( 0),O( 0),O( 0),

    /* rate 15 */
    O( 0),O( 0),O( 0),O( 0),

    /* 16 dummy rates (same as 15 3) */
    O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
    O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
};
#undef O

#define O(a) (a*RATE_STEPS)
const unsigned char YM2413CORE::eg_rate_select[16+64+16]={  /* Envelope Generator rates (16 + 64 rates + 16 RKS) */
    /* 16 infinite time rates */
    O(14),O(14),O(14),O(14),O(14),O(14),O(14),O(14),
    O(14),O(14),O(14),O(14),O(14),O(14),O(14),O(14),

    /* rates 00-12 */
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),
    O( 0),O( 1),O( 2),O( 3),

    /* rate 13 */
    O( 4),O( 5),O( 6),O( 7),

    /* rate 14 */
    O( 8),O( 9),O(10),O(11),

    /* rate 15 */
    O(12),O(12),O(12),O(12),

    /* 16 dummy rates (same as 15 3) */
    O(12),O(12),O(12),O(12),O(12),O(12),O(12),O(12),
    O(12),O(12),O(12),O(12),O(12),O(12),O(12),O(12),
};
#undef O

const u8 YM2413CORE::lfo_am_table[LFO_AM_TAB_ELEMENTS] = {
    0,0,0,0,0,0,0,
    1,1,1,1,
    2,2,2,2,
    3,3,3,3,
    4,4,4,4,
    5,5,5,5,
    6,6,6,6,
    7,7,7,7,
    8,8,8,8,
    9,9,9,9,
    10,10,10,10,
    11,11,11,11,
    12,12,12,12,
    13,13,13,13,
    14,14,14,14,
    15,15,15,15,
    16,16,16,16,
    17,17,17,17,
    18,18,18,18,
    19,19,19,19,
    20,20,20,20,
    21,21,21,21,
    22,22,22,22,
    23,23,23,23,
    24,24,24,24,
    25,25,25,25,
    26,26,26,
    25,25,25,25,
    24,24,24,24,
    23,23,23,23,
    22,22,22,22,
    21,21,21,21,
    20,20,20,20,
    19,19,19,19,
    18,18,18,18,
    17,17,17,17,
    16,16,16,16,
    15,15,15,15,
    14,14,14,14,
    13,13,13,13,
    12,12,12,12,
    11,11,11,11,
    10,10,10,10,
    9,9,9,9,
    8,8,8,8,
    7,7,7,7,
    6,6,6,6,
    5,5,5,5,
    4,4,4,4,
    3,3,3,3,
    2,2,2,2,
    1,1,1,1
};

const i8 YM2413CORE::lfo_pm_table[8*8] = {
    /* FNUM2/FNUM = 0 00xxxxxx (0x0000) */
    0, 0, 0, 0, 0, 0, 0, 0,

    /* FNUM2/FNUM = 0 01xxxxxx (0x0040) */
    1, 0, 0, 0,-1, 0, 0, 0,

    /* FNUM2/FNUM = 0 10xxxxxx (0x0080) */
    2, 1, 0,-1,-2,-1, 0, 1,

    /* FNUM2/FNUM = 0 11xxxxxx (0x00C0) */
    3, 1, 0,-1,-3,-1, 0, 1,

    /* FNUM2/FNUM = 1 00xxxxxx (0x0100) */
    4, 2, 0,-2,-4,-2, 0, 2,

    /* FNUM2/FNUM = 1 01xxxxxx (0x0140) */
    5, 2, 0,-2,-5,-2, 0, 2,

    /* FNUM2/FNUM = 1 10xxxxxx (0x0180) */
    6, 3, 0,-3,-6,-3, 0, 3,

    /* FNUM2/FNUM = 1 11xxxxxx (0x01C0) */
    7, 3, 0,-3,-7,-3, 0, 3,
};

unsigned char YM2413CORE::table[19][8] = {
    /* MULT  MULT modTL DcDmFb AR/DR AR/DR SL/RR SL/RR */
    /*   0     1     2     3     4     5     6    7    */
    {0x49, 0x4c, 0x4c, 0x12, 0x00, 0x00, 0x00, 0x00 },  //0

    {0x61, 0x61, 0x1e, 0x17, 0xf0, 0x78, 0x00, 0x17 },  //1
    {0x13, 0x41, 0x1e, 0x0d, 0xd7, 0xf7, 0x13, 0x13 },  //2
    {0x13, 0x01, 0x99, 0x04, 0xf2, 0xf4, 0x11, 0x23 },  //3
    {0x21, 0x61, 0x1b, 0x07, 0xaf, 0x64, 0x40, 0x27 },  //4

    //{0x22, 0x21, 0x1e, 0x09, 0xf0, 0x76, 0x08, 0x28 },  //5
    {0x22, 0x21, 0x1e, 0x06, 0xf0, 0x75, 0x08, 0x18 },  //5

    //{0x31, 0x22, 0x16, 0x09, 0x90, 0x7f, 0x00, 0x08 },  //6
    {0x31, 0x22, 0x16, 0x05, 0x90, 0x71, 0x00, 0x13 },  //6

    {0x21, 0x61, 0x1d, 0x07, 0x82, 0x80, 0x10, 0x17 },  //7
    {0x23, 0x21, 0x2d, 0x16, 0xc0, 0x70, 0x07, 0x07 },  //8
    {0x61, 0x61, 0x1b, 0x06, 0x64, 0x65, 0x10, 0x17 },  //9

    //{0x61, 0x61, 0x0c, 0x08, 0x85, 0xa0, 0x79, 0x07 },  //A
    {0x61, 0x61, 0x0c, 0x18, 0x85, 0xf0, 0x70, 0x07 },  //A

    {0x23, 0x01, 0x07, 0x11, 0xf0, 0xa4, 0x00, 0x22 },  //B
    {0x97, 0xc1, 0x24, 0x07, 0xff, 0xf8, 0x22, 0x12 },  //C

    //{0x61, 0x10, 0x0c, 0x08, 0xf2, 0xc4, 0x40, 0xc8 },  //D
    {0x61, 0x10, 0x0c, 0x05, 0xf2, 0xf4, 0x40, 0x44 },  //D

    {0x01, 0x01, 0x55, 0x03, 0xf3, 0x92, 0xf3, 0xf3 },  //E
    {0x61, 0x41, 0x89, 0x03, 0xf1, 0xf4, 0xf0, 0x13 },  //F

    /* drum instruments definitions */
    /* MULTI MULTI modTL  xxx  AR/DR AR/DR SL/RR SL/RR */
    /*   0     1     2     3     4     5     6    7    */
    {0x01, 0x01, 0x16, 0x00, 0xfd, 0xf8, 0x2f, 0x6d },/* BD(multi verified, modTL verified, mod env - verified(close), carr. env verifed) */
    {0x01, 0x01, 0x00, 0x00, 0xd8, 0xd8, 0xf9, 0xf8 },/* HH(multi verified), SD(multi not used) */
    {0x05, 0x01, 0x00, 0x00, 0xf8, 0xba, 0x49, 0x55 },/* TOM(multi,env verified), TOP CYM(multi verified, env verified) */
};

const unsigned char YM2413CORE::eg_inc[15*RATE_STEPS]={
    /*cycle:0 1  2 3  4 5  6 7*/

    /* 0 */ 0,1, 0,1, 0,1, 0,1, /* rates 00..12 0 (increment by 0 or 1) */
    /* 1 */ 0,1, 0,1, 1,1, 0,1, /* rates 00..12 1 */
    /* 2 */ 0,1, 1,1, 0,1, 1,1, /* rates 00..12 2 */
    /* 3 */ 0,1, 1,1, 1,1, 1,1, /* rates 00..12 3 */

    /* 4 */ 1,1, 1,1, 1,1, 1,1, /* rate 13 0 (increment by 1) */
    /* 5 */ 1,1, 1,2, 1,1, 1,2, /* rate 13 1 */
    /* 6 */ 1,2, 1,2, 1,2, 1,2, /* rate 13 2 */
    /* 7 */ 1,2, 2,2, 1,2, 2,2, /* rate 13 3 */

    /* 8 */ 2,2, 2,2, 2,2, 2,2, /* rate 14 0 (increment by 2) */
    /* 9 */ 2,2, 2,4, 2,2, 2,4, /* rate 14 1 */
    /*10 */ 2,4, 2,4, 2,4, 2,4, /* rate 14 2 */
    /*11 */ 2,4, 4,4, 2,4, 4,4, /* rate 14 3 */

    /*12 */ 4,4, 4,4, 4,4, 4,4, /* rates 15 0, 15 1, 15 2, 15 3 (increment by 4) */
    /*13 */ 8,8, 8,8, 8,8, 8,8, /* rates 15 2, 15 3 for attack */
    /*14 */ 0,0, 0,0, 0,0, 0,0, /* infinity rates for attack and decay(s) */
};

#define SC(db) (u32) ( db * (1.0/ENV_STEP) )
const u32 YM2413CORE::sl_tab[16] = {
    SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
    SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(15)
};
#undef SC

/* multiple table */
#define ML 2
const double YM2413CORE::mul_tab[16] = {
/* 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,10,12,12,15,15 */
   0.50*ML, 1.00*ML, 2.00*ML, 3.00*ML, 4.00*ML, 5.00*ML, 6.00*ML, 7.00*ML,
   8.00*ML, 9.00*ML,10.00*ML,10.00*ML,12.00*ML,12.00*ML,15.00*ML,15.00*ML
};
#undef ML

void YM2413CORE::initTlTable() {
    signed int n;
    double m;

    for (signed int x=0; x<TL_RES_LEN; x++) {
        m = (1<<16) / pow(2, (x+1) * (ENV_STEP/4.0) / 8.0);
        m = floor(m);

        n = (int)m;
        n >>= 4;
        if (n&1)    n = (n>>1)+1;
        else        n = n>>1;

        tl_tab[ x*2 + 0 ] = n;
        tl_tab[ x*2 + 1 ] = -tl_tab[ x*2 + 0 ];

        for (signed int i=1; i<11; i++) {
            tl_tab[ x*2+0 + i*2*TL_RES_LEN ] =  tl_tab[ x*2+0 ]>>i;
            tl_tab[ x*2+1 + i*2*TL_RES_LEN ] = -tl_tab[ x*2+0 + i*2*TL_RES_LEN ];
        }
    }
}

void YM2413CORE::initSinTable() {
    signed int n;
    double o, m;

    for (signed int i=0; i<SIN_LEN; i++) {
        m = sin( ((i*2)+1) * M_PI / SIN_LEN ); /* non-standard sinus */

        if (m>0.0)  o = 8*log(1.0/m)/log(2);  /* convert to 'decibels' */
        else        o = 8*log(-1.0/m)/log(2);  /* convert to 'decibels' */

        o = o / (ENV_STEP/4);

        n = (int)(2.0*o);
        if (n&1)    n = (n>>1)+1;
        else        n = n>>1;

        sin_tab[ i ] = n*2 + (m>=0.0 ? 0: 1 ); /* waveform 0: standard sinus  */

        if (i & (1<<(SIN_BITS-1)) ) sin_tab[1*SIN_LEN+i] = TL_TAB_LEN;
        else                        sin_tab[1*SIN_LEN+i] = sin_tab[i];
    }
}

