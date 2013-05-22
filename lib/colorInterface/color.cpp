#include "color.h"
#include <math.h>

void Color::set_filter(int _type, u8 cable) {
	type = (FilterType)_type;
	if (active_filter) {
		delete(active_filter);
		active_filter = 0;
	}

	switch(type) {
		case Direct:
			active_filter = new Filter_direct(colorTable);
			break;
		case NTSC:
			active_filter = new Filter_ntsc(colorTable, cable);
			break;
	}
	active_filter->init();
}

void Color::update() {
	if (colortable_updated) return;

	u32 col;
	u8 r, g, b;
	int i;

	//Master System (bgr222) -> PC (rgb888)
    for(i = 0; i < 64; i++) {
		col = colorConverter( (i >> 4) & 3 );
		col |= colorConverter( (i >> 2) & 3 ) << 8;
		col |= colorConverter(i & 3) << 16;

		r = (col >> 16) & 0xff;
		g = (col >>  8) & 0xff;
		b = (col      ) & 0xff;

		//c' = contrast * c^gamma + brightness
		adjust_gamma(r); adjust_contrast(r); adjust_brightness(r); 
		adjust_gamma(g); adjust_contrast(g); adjust_brightness(g);
		adjust_gamma(b); adjust_contrast(b); adjust_brightness(b);

		colorTable[SMS]._RGB32[i] = (r << 16) | (g << 8) | (b);

		r >>= 3;
		g >>= 2;
		b >>= 3;
		colorTable[SMS]._RGB16[i] = (r << 11) | (g << 5) | (b);
	}

	//original TMS colors
	for(i = 0; i < 16; i++) {
		r = tms[i].red;
		g = tms[i].green;
		b = tms[i].blue;

		//c' = contrast * c^gamma + brightness
		adjust_gamma(r); adjust_contrast(r); adjust_brightness(r); 
		adjust_gamma(g); adjust_contrast(g); adjust_brightness(g);
		adjust_gamma(b); adjust_contrast(b); adjust_brightness(b);

		colorTable[TMS]._RGB32[i] = (r << 16) | (g << 8) | (b);

		r >>= 3;
		g >>= 2;
		b >>= 3;
		colorTable[TMS]._RGB16[i] = (r << 11) | (g << 5) | (b);
	}

    //sms tms colors
	for(i = 0; i < 16; i++) {
		r = tms_sms[i].red;
		g = tms_sms[i].green;
		b = tms_sms[i].blue;

		//c' = contrast * c^gamma + brightness
		adjust_gamma(r); adjust_contrast(r); adjust_brightness(r); 
		adjust_gamma(g); adjust_contrast(g); adjust_brightness(g);
		adjust_gamma(b); adjust_contrast(b); adjust_brightness(b);

		colorTable[SMS_TMS]._RGB32[i] = (r << 16) | (g << 8) | (b);

		r >>= 3;
		g >>= 2;
		b >>= 3;
		colorTable[SMS_TMS]._RGB16[i] = (r << 11) | (g << 5) | (b);
	}

    //game gear (bgr444) -> PC (rgb888)
    for(i = 0; i < 4096; i++) {
        col  = ((i & 0xf) << 20) | ((i & 0xf) << 16)
                | ((i & 0xf0) <<  8) | ((i & 0xf0) <<  4)
                | ((i & 0xf00) >>  4) | ((i & 0xf00) >>  8);

        r = (col >> 16) & 0xff;
        g = (col >>  8) & 0xff;
        b = (col      ) & 0xff;

        //c' = contrast * c^gamma + brightness
        adjust_gamma(r); adjust_contrast(r); adjust_brightness(r);
        adjust_gamma(g); adjust_contrast(g); adjust_brightness(g);
        adjust_gamma(b); adjust_contrast(b); adjust_brightness(b);

        colorTable[GG]._RGB32[i] = (r << 16) | (g << 8) | (b);

        r >>= 3;
        g >>= 2;
        b >>= 3;
        colorTable[GG]._RGB16[i] = (r << 11) | (g << 5) | (b);
    }

	if (!active_filter) {
		active_filter = new Filter_direct(colorTable);
	}
	active_filter->init();
	colortable_updated = true;
}

u8 Color::colorConverter(u8 smsColor) {
	if (smsColor == 0) return 0;
	if (smsColor == 1) return 85;
	if (smsColor == 2) return 170;
	return 255;
}

void Color::adjust_contrast(u8& channel) {
   if (contrast == 0) return;

   double lmin =   0.0 - double(contrast);
   double lmax = 255.0 + double(contrast);

   i16 result = i16(((lmax - lmin) / (lmax + lmin)) * double(channel) + lmin);
   channel = (u8)_minmax<i16>(result, 0, 255);
}

void Color::adjust_gamma(u8& channel) {
   if (gamma == 100) return;
   i16 result = i16(pow((double(channel) / 255.0), 1.0 / (double(gamma) / 100.0)) * 255.0);
   channel = (u8)_minmax<i16>(result, 0, 255);
}

void Color::adjust_brightness(u8& channel) {
   if (brightness == 0) return;
   i16 result = channel + brightness;
   channel = (u8)_minmax<i16>(result, 0, 255);
}
