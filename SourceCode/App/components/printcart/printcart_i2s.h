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

typedef void (*i2s_parallel_refill_buffer_cb_t)(void *buff, int len, void *arg);

typedef enum {
	I2S_PARALLEL_BITS_8=8,
	I2S_PARALLEL_BITS_16=16,
	I2S_PARALLEL_BITS_32=32,
} i2s_parallel_cfg_bits_t;

typedef struct {
	int gpio_bus[24];							 
	int clkspeed_hz;							 
	i2s_parallel_cfg_bits_t bits;				 
	int bufsz;									 
	i2s_parallel_refill_buffer_cb_t refill_cb;	 
	void *refill_cb_arg;						 
} i2s_parallel_config_t;

void i2s_parallel_setup(const i2s_parallel_config_t *cfg);

void i2s_parallel_start();
void loop();
void setup(const i2s_parallel_config_t *cfg);

#define PIN_NUM_CART_D2 18
#define PIN_NUM_CART_D1 10
#define PIN_NUM_CART_D3 8
#define PIN_NUM_CART_CSYNC 9
#define PIN_NUM_CART_S2 11
#define PIN_NUM_CART_S4 47
#define PIN_NUM_CART_S1 21
#define PIN_NUM_CART_S5 14
#define PIN_NUM_CART_DCLK 13
#define PIN_NUM_CART_S3 12
#define PIN_NUM_CART_F3 48
#define PIN_NUM_CART_F5 17

#pragma once

#if __has_include (<hal/lcd_ll.h>)

#include <sdkconfig.h>

#include <esp_intr_alloc.h>

#include <esp_err.h>
#include <esp_log.h>

#include <driver/gpio.h>
#include <soc/gpio_sig_map.h>

#include <hal/gpio_ll.h>
#include <hal/lcd_hal.h>

#include <string.h>
#include <math.h>

#include <stdint.h>

#if (ESP_IDF_VERSION_MAJOR == 5)
#include <esp_private/periph_ctrl.h>
#else
#include <driver/periph_ctrl.h>
#endif

#include <esp_private/gdma.h>
#include <esp_rom_gpio.h>
#include <hal/dma_types.h>
#include <hal/gpio_hal.h>

#include <hal/lcd_ll.h>
#include <soc/lcd_cam_reg.h>
#include <soc/lcd_cam_struct.h>

#include <esp_heap_caps.h>
#include <esp_heap_caps_init.h>

#if __has_include (<esp_private/periph_ctrl.h>)
 #include <esp_private/periph_ctrl.h>
#else
 #include <driver/periph_ctrl.h>
#endif

#if __has_include(<esp_arduino_version.h>)
 #include <esp_arduino_version.h>
#endif

#define HUB75_DMA_DESCRIPTOR_T dma_descriptor_t

    typedef struct 
    {

      uint32_t bus_freq; 
      int8_t pin_wr; 
      int8_t pin_rd; 
      int8_t pin_rs; 
      bool   invert_pclk; 
      bool   psram_clk_hack; 
      union
      {
        int8_t pin_data[16];
        struct
        {
          int8_t pin_d0;
          int8_t pin_d1;
          int8_t pin_d2;
          int8_t pin_d3;
          int8_t pin_d4;
          int8_t pin_d5;
          int8_t pin_d6;
          int8_t pin_d7;
          int8_t pin_d8;
          int8_t pin_d9;
          int8_t pin_d10;
          int8_t pin_d11;
          int8_t pin_d12;
          int8_t pin_d13;
          int8_t pin_d14;
          int8_t pin_d15;
        };
      };
    } config_t;

    void  config(const config_t* config);

    bool init(void) ;

    void release(void) ;

    void enable_double_dma_desc();
    bool allocate_dma_desc_memory(size_t len);

    void dma_transfer_start();
    void dma_transfer_stop();

#endif
