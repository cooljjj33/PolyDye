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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_attr.h"
#include "printcart_buffer_filler.h"
#include "printcart_genwaveform.h"
#include <string.h>

int printcart_mem_words_used=0;

static uint16_t waveform_tpl_color_a[]={
	0x0,0xc001,0xc101,0xc142,0x42,0x4001,0x4101,0x4112,0x12,0x4001,0x4101,0x4302,0x602,0x4401,0x4501,0x4522,0x422,0x402,0x4000,0x4000,0x4080,0x880,0x800,0x800,0x800,0x800,0x800

};
static uint16_t waveform_tpl_color_b[]={
////0x0,0x0,0xc100,0xc141,0xc041,0x2,0x4102,0x4111,0x4011,0x2,0x4102,0x4301,0x4601,0x402,0x4502,0x4521,0x4021,0x2,0x2,0x4000,0x4080,0x4880,0x800,0x800,0x800,0x0,0x0
0x0,0x0,0x8100,0xc141,0xc041,0x4002,0x102,0x111,0x4011,0x4002,0x102,0x4301,0x4201,0x4402,0x502,0x4521,0x4421,0x4402,0x402,0x0,0x4080,0x4880,0x4800,0x800,0x800,0x800,0x800

};
static uint16_t waveform_tpl_black_a[]={

    0x0,0xc001,0xc101,0xc142,0x42,0xc001,0xc101,0xc112,0x12,0x4001,0x4101,0x4302,0x602,0x401,0x501,0x522,0x422,0x402,0x4000,0x4000,0x4080,0x880,0x800,0x800,0x800,0x800,0x800

    };
static uint16_t waveform_tpl_black_b[]={

0x0,0x0,0x4500,0xc541,0xc041,0x8002,0x902,0xc911,0xc011,0x8002,0x4102,0x4301,0x4601,0x402,0x502,0x521,0x421,0x402,0x2,0x4002,0x4080,0x4880,0x800,0x800,0x800,0x800,0x800

};

typedef struct {
	uint16_t *data;
	int len;
} waveform_desc_t;

const waveform_desc_t waveforms[]={
	{waveform_tpl_color_a, sizeof(waveform_tpl_color_a)/2},
	{waveform_tpl_color_b, sizeof(waveform_tpl_color_b)/2},
	{waveform_tpl_black_a, sizeof(waveform_tpl_black_a)/2},
	{waveform_tpl_black_b, sizeof(waveform_tpl_black_b)/2},
};

static uint16_t *selected_waveform_tpl=waveform_tpl_color_a;
static int selected_waveform_len=sizeof(waveform_tpl_color_a)/2;

int clear_waveform_ct=0;

void printcart_select_waveform(enum printcart_buffer_filler_waveform_type_en waveform_id) {
	clear_waveform_ct=3;
	selected_waveform_tpl=waveforms[waveform_id].data;
	selected_waveform_len=waveforms[waveform_id].len;
}

void IRAM_ATTR printcart_buffer_filler_fn(void *buf, int len, void *arg) {
	QueueHandle_t pixq=(QueueHandle_t)arg;
	portBASE_TYPE high_priority_task_awoken = 0;

	if (clear_waveform_ct) {
		clear_waveform_ct--;
		memset(buf, 0, len);
	}

	uint8_t nozdata[PRINTCART_NOZDATA_SZ];

	if (xQueueReceiveFromISR(pixq, nozdata, &high_priority_task_awoken)==pdFALSE) {

		memset(nozdata, 0, PRINTCART_NOZDATA_SZ);
	} 
	
	printcart_mem_words_used=printcart_generate_waveform((uint16_t*)buf, selected_waveform_tpl, nozdata, selected_waveform_len);

	if (high_priority_task_awoken == pdTRUE) {
		portYIELD_FROM_ISR();
	}
}
