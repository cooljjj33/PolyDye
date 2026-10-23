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

#pragma once

#define PRINTCART_NOZDATA_SZ (28*3)

#define PRINTCART_COLOR_C 0
#define PRINTCART_COLOR_M 1
#define PRINTCART_COLOR_Y 2

void printcart_fire_nozzle_color(uint8_t *l, int p, int color);

void printcart_fire_nozzle_black(uint8_t *l, int p, int row);

int printcart_generate_waveform(uint16_t *w, const uint16_t *tp, const uint8_t *nozdata, int l);
