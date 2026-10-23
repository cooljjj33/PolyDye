/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0 AND Beerware
 * 
 * CMY Labs PTY LTD's original copyrightable additions are NCL-1.0 ONLY.
 * Retained upstream expression remains under Beerware.
 * The licenses apply to their respective contributions, not as alternatives.
 * Retained upstream license identifiers below describe upstream material.
 * Modified in this project; upstream notices are preserved.
 * Upstream: Jeroen Domburg, printercart_simple.
 * Reference revision: 4f6bbfad0e1d40917788b15a196d17a393c05139.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include "rom/ets_sys.h"
#include "esp_attr.h"
#include <string.h>
#include "printcart_genwaveform.h"
#include "rom/cache.h"
#include "esp_err.h"
void IRAM_ATTR printcart_fire_nozzle_color(uint8_t *l, int p, int color) {

    int bo[3][14]={

		 {8,13,4,9,0,5,10,1,6,11,2,7,12,3},
	     {11,2,7,12,3,8,13,4,9,0,5,10,1,6},
		 {0,5,10,1,6,11,2,7,12,3,8,13,4,9}

    };
    if (p>(28*14) || p<0) return;
    int byteno=bo[color][p%14];
    int bitno=p/14;
    l[byteno+(14*color)]|=(1<<bitno);
}

typedef struct {
	int c;
	int bit;
	int order;
} bw_nozinfo_t;

const bw_nozinfo_t ni[]={
	{2,0,1}, {2,1,1}, {1,0,1}, {1,1,1},
	{0,0,1}, {0,1,1}, {2,4,1}, {2,5,1},
	{1,4,1}, {1,5,1}, {0,4,1}, {0,5,1},
	{2,2,0}, {2,3,0}, {1,2,0}, {1,3,0},
	{0,2,0}, {0,3,0}, {2,6,0}, {2,7,0},
	{1,6,0}, {1,7,0}, {0,6,0}, {0,7,0},
};

void IRAM_ATTR printcart_fire_nozzle_black(uint8_t *l, int p, int row) {
	if (row) p+=168;
	int j=p/14;
	int k=13-(p%14);

	const int bo[2][14]={

		{3,11,5,4,12,10,2,8,0,6,13,7,1,9},
		{9,3,11,5,4,12,10,2,8,0,6,13,7,1},

	};

	l[ni[j].c*14 + bo[ni[j].order][k]] |= (1<<ni[j].bit);
}

static IRAM_ATTR inline void write_signals(uint16_t *buf, int pos, uint16_t val) {

	buf[pos]=val;

}

#define TP_BIT_TOGGLE1 (1<<0)
#define TP_BIT_TOGGLE2 (1<<1)
#define TP_S2 (1<<4)
#define TP_S4 (1<<5)
#define TP_S1 (1<<6)
#define TP_S5 (1<<7)
#define TP_DCLK (1<<8)
#define TP_S3 (1<<9)
#define TP_F3 (1<<10)
#define TP_F5 (1<<11)
#define TP_CSYNC_LAST (1<<14)
#define TP_CSYNC_NORM (1<<15)

#define OUT_D1 (1<<0)
#define OUT_D2 (1<<1)
#define OUT_D3 (1<<2)
#define OUT_CSYNC (1<<3)
#define OUT_S2 (1<<4)
#define OUT_S4 (1<<5)
#define OUT_S1 (1<<6)
#define OUT_S5 (1<<7)
#define OUT_DCLK (1<<8)
#define OUT_S3 (1<<9)
#define OUT_F3 (1<<10)
#define OUT_F5 (1<<11)

int IRAM_ATTR printcart_generate_waveform(uint16_t *w, const uint16_t *tp, const uint8_t *nozdata, int l) {
	const uint8_t *d1data=&nozdata[0];
	const uint8_t *d2data=&nozdata[14];
	const uint8_t *d3data=&nozdata[28];
	int p=0;

	int is_empty=1;
	for (int i=0; i<PRINTCART_NOZDATA_SZ; i++) {
		if (nozdata[i]!=0) {
			is_empty=0;
			break;
		}
	}

	for (int j=0; j<14; j++) {
		uint8_t bit=0;
		int ov=0;

		uint16_t mask=0xffff;
		mask&=~(OUT_CSYNC|OUT_D1|OUT_D2|OUT_D3);  

		int power=(d1data[j]|d2data[j]|d3data[j]);
		if ((power&0x0f)==0x00) mask&=~OUT_F3;
		if ((power&0xf0)==0x00) mask&=~OUT_F5;

		uint16_t csync_sel=0;
		if (j==13) {  
			csync_sel=TP_CSYNC_LAST;
		} else {
			csync_sel=TP_CSYNC_NORM;
		}

		for (int i=0; i<l; i++) {
			if (is_empty) {

				write_signals(w, p++, 0);
				continue;
			}
			uint16_t v=tp[i];

			if ((v&(TP_BIT_TOGGLE1|TP_BIT_TOGGLE2))!=ov) {
				if (!bit) {
					bit=1;  
				} else {
					bit<<=1;  
				}
				ov=v&(TP_BIT_TOGGLE1|TP_BIT_TOGGLE2);
			}
			int en=v&(TP_BIT_TOGGLE1|TP_BIT_TOGGLE2);  

			v&=mask;  

			if (en) {
				if (!(d1data[j]&bit)) v|=OUT_D1;
				if (!(d2data[j]&bit)) v|=OUT_D2;
				if (!(d3data[j]&bit)) v|=OUT_D3;
			}

			if (v&csync_sel) v|=OUT_CSYNC;
			write_signals(w, p++, v);
		}
		p+=8;  
	}

	return (p+1)&~1;
}
