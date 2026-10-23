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
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "soc/io_mux_reg.h"
#include "driver/gpio.h"
#include "soc/gpio_periph.h"
#include "rom/gpio.h"

#include <esp_private/gdma.h>

#include <hal/dma_types.h>
#include <hal/gpio_hal.h>
#include <soc/lcd_cam_struct.h>
#include <esp_rom_gpio.h>
#include "printcart_i2s.h"

#include "esp_err.h"
#include "esp_log.h"

gdma_channel_handle_t dma_chan;  
dma_descriptor_t desc;           
uint8_t data[8][312];            
typedef struct {
	volatile dma_descriptor_t *dmadesc_a, *dmadesc_b;
	int desccount_a, desccount_b;
	i2s_parallel_refill_buffer_cb_t refill_cb;
	void *refill_cb_arg;
	int bufsz;
} i2s_parallel_state_t;

static i2s_parallel_state_t *i2s_state[2]={NULL, NULL};

#define DMA_MAX (8096-4)

static int calc_needed_dma_descs_for(int memsize) {
	return (memsize+DMA_MAX-1)/DMA_MAX;
}

static IRAM_ATTR bool dma_callback(gdma_channel_handle_t dma_chan,
                                   gdma_event_data_t *event_data, void *user_data) {

  volatile gdma_event_data_t* dev = event_data;
	int devno=0;

  LCD_CAM.lcd_misc.val = 0;
		dma_descriptor_t *finish_desc = (dma_descriptor_t*)dev->tx_eof_desc_addr;
		i2s_state[devno]->refill_cb((void*)finish_desc->buffer, i2s_state[devno]->bufsz, i2s_state[devno]->refill_cb_arg);

  LCD_CAM.lcd_user.lcd_start = 1;
  return true;
}

static int fill_dma_desc(volatile dma_descriptor_t *dmadesc, void *memory, int size) {
	int n=0;
	int len=size;
	uint8_t *data=((uint8_t*)memory);
	while(len) {
		int dmalen=len;
		if (dmalen>DMA_MAX) dmalen=DMA_MAX;
		dmadesc[n].dw0.size=dmalen;
		dmadesc[n].dw0.length=dmalen;
		dmadesc[n].buffer=data;
		dmadesc[n].dw0.suc_eof=0;

		dmadesc[n].dw0.err_eof=0;
		dmadesc[n].dw0.owner=1;
		dmadesc[n].next=(dma_descriptor_t*)&dmadesc[n+1];

		len-=dmalen;
		data+=dmalen;
		n++;
	}
	return n;

}

void setup(  const i2s_parallel_config_t *cfg) {

  periph_module_enable(PERIPH_LCD_CAM_MODULE);
  periph_module_reset(PERIPH_LCD_CAM_MODULE);

  LCD_CAM.lcd_user.lcd_reset = 1;
  esp_rom_delay_us(100);

  LCD_CAM.lcd_clock.clk_en = 1;              
  LCD_CAM.lcd_clock.lcd_clk_sel = 1;         
  LCD_CAM.lcd_clock.lcd_ck_out_edge = 0;     
  LCD_CAM.lcd_clock.lcd_ck_idle_edge = 0;    
  LCD_CAM.lcd_clock.lcd_clk_equ_sysclk = 0;  
  LCD_CAM.lcd_clock.lcd_clkm_div_num = 1;  
  LCD_CAM.lcd_clock.lcd_clkm_div_a = 0;      
  LCD_CAM.lcd_clock.lcd_clkm_div_b = 0;

  LCD_CAM.lcd_clock.lcd_clkcnt_n = 40000000/(3333333)/2;       

  LCD_CAM.lcd_ctrl.lcd_rgb_mode_en = 0;     
  LCD_CAM.lcd_rgb_yuv.lcd_conv_bypass = 0;  
  LCD_CAM.lcd_misc.lcd_next_frame_en = 0;   
  LCD_CAM.lcd_data_dout_mode.val = 0;       
  LCD_CAM.lcd_user.lcd_always_out_en = 1;   
  LCD_CAM.lcd_user.lcd_8bits_order = 0;     
  LCD_CAM.lcd_user.lcd_bit_order = 0;       
  LCD_CAM.lcd_user.lcd_2byte_en = 1;        
  LCD_CAM.lcd_user.lcd_dummy = 0;           
  LCD_CAM.lcd_user.lcd_dummy_cyclelen = 0;  
  LCD_CAM.lcd_user.lcd_cmd = 0;             

  LCD_CAM.lcd_misc.val = 0;

  for (int i = 0; i < 16; i++) {
    if (cfg->gpio_bus[i] != -1){
      esp_rom_gpio_connect_out_signal(cfg->gpio_bus[i], LCD_DATA_OUT0_IDX+i, false, false);
      PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cfg->gpio_bus[i]], PIN_FUNC_GPIO);
      if (i > 9) {
        gpio_set_drive_capability((gpio_num_t)cfg->gpio_bus[i],   GPIO_DRIVE_CAP_3); 
      } else {
        gpio_set_drive_capability((gpio_num_t)cfg->gpio_bus[i],   GPIO_DRIVE_CAP_1); 
      }
    }
  }

 ESP_LOGI("ALOOC", "Free heap: %d",   heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  ESP_LOGI("ALOOC", "Free SPIRAM: %d", heap_caps_get_free_size(MALLOC_CAP_SPIRAM)); 

	void *mema=heap_caps_malloc(cfg->bufsz, MALLOC_CAP_DMA);
	void *memb=heap_caps_malloc(cfg->bufsz, MALLOC_CAP_DMA);
	memset(mema, 0, cfg->bufsz);
	memset(memb, 0, cfg->bufsz);

	i2s_state[0]=malloc(sizeof(i2s_parallel_state_t));
	i2s_parallel_state_t *st=i2s_state[0];
	st->desccount_a=calc_needed_dma_descs_for(cfg->bufsz);
	st->desccount_b=calc_needed_dma_descs_for(cfg->bufsz);
	st->dmadesc_a=heap_caps_malloc(st->desccount_a*sizeof(dma_descriptor_t), MALLOC_CAP_DMA);
	st->dmadesc_b=heap_caps_malloc(st->desccount_b*sizeof(dma_descriptor_t), MALLOC_CAP_DMA);

	int cta=fill_dma_desc(st->dmadesc_a, mema, cfg->bufsz);
	int ctb=fill_dma_desc(st->dmadesc_b, memb, cfg->bufsz);

	st->dmadesc_a[cta-1].dw0.suc_eof  = 1;
	st->dmadesc_b[ctb-1].dw0.suc_eof =1;
	st->dmadesc_a[cta-1].next=(dma_descriptor_t*)&st->dmadesc_b[0];
	st->dmadesc_b[ctb-1].next=(dma_descriptor_t*)&st->dmadesc_a[0];

	st->refill_cb=cfg->refill_cb;
	st->refill_cb_arg=cfg->refill_cb_arg;
	st->bufsz=cfg->bufsz;

  gdma_channel_alloc_config_t dma_chan_config = {
    .sibling_chan = NULL,
    .direction = GDMA_CHANNEL_DIRECTION_TX,
    .flags = {
      .reserve_sibling = 0
    }
  };
  gdma_new_channel(&dma_chan_config, &dma_chan);
  gdma_connect(dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
  gdma_strategy_config_t strategy_config = {
    .owner_check = false,
    .auto_update_desc = false
  };
  gdma_apply_strategy(dma_chan, &strategy_config);

  gdma_tx_event_callbacks_t tx_cbs = {
    .on_trans_eof = dma_callback
  };
  gdma_register_tx_event_callbacks(dma_chan, &tx_cbs, NULL);

}

void i2s_parallel_start() {

      int a= xPortGetCoreID();

    ESP_LOGI("Printinit", "Running on %d", a);
  	int devno=0;

	i2s_state[devno]->refill_cb((void*)i2s_state[devno]->dmadesc_a[0].buffer, i2s_state[devno]->bufsz, i2s_state[devno]->refill_cb_arg);
	i2s_state[devno]->refill_cb((void*)i2s_state[devno]->dmadesc_b[0].buffer, i2s_state[devno]->bufsz, i2s_state[devno]->refill_cb_arg);

  gdma_reset(dma_chan);                  
  LCD_CAM.lcd_user.lcd_dout = 1;         
  LCD_CAM.lcd_user.lcd_update = 1;       
  LCD_CAM.lcd_misc.lcd_afifo_reset = 1;  

  gdma_start(dma_chan, (intptr_t)&i2s_state[0]->dmadesc_a[0]);  
  esp_rom_delay_us(1);                   
  LCD_CAM.lcd_user.lcd_start = 1;         
}
