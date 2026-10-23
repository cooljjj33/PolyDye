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

#include <stdint.h>

#pragma once

extern int printcart_mem_words_used;

enum printcart_buffer_filler_waveform_type_en {
	PRINTCART_WAVEFORM_COLOR_A=0,		 
	PRINTCART_WAVEFORM_COLOR_B,			 
	PRINTCART_WAVEFORM_BLACK_A,			 
	PRINTCART_WAVEFORM_BLACK_B,			 
};

void printcart_select_waveform(enum printcart_buffer_filler_waveform_type_en waveform_id);

void printcart_buffer_filler_fn(void *buf, int len, void *arg);
