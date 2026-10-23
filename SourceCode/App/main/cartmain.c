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

#define CART_IS_COLOR 1

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <driver/gpio.h>
#include <string.h>
#include "printcart_buffer_filler.h"
#include "printcart_genwaveform.h"
#include "printcart_i2s.h"

#include <inttypes.h>

#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include <driver/i2c.h>

#include "decode_image.h"
#include "esp_timer.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "globalPrinter.h"
#include "esp_task_wdt.h"

#define BUFFER_SIZE 13
#define SD_BUFFER_SIZE 8 * 1024
typedef struct
{
	uint16_t *buffer[BUFFER_SIZE];
	size_t head;
	size_t tail;
	size_t count;
	bool freeze;

} ImageRingBuffer;

ImageRingBuffer ring_buffer;

QueueHandle_t nozdata_queue;

#define WAVEFORM_DMALEN 3500

#define MOUNT_POINT "/sdcard\0"

uint16_t *pixels;

#ifdef CART_IS_COLOR
#define PICTURE_W 1666
#define PICTURE_H 144  
#endif
#ifdef CART_IS_BLACK
#define PICTURE_W 1666
#define PICTURE_H 168  
#endif

bool thereIsFuckenNothing = false;

static inline uint8_t get_bgnd_pixel(int x, int y, int color)
{
	x = x % PICTURE_W;
	if (x < 0)
		x += PICTURE_W;
	if (y < 0 || y >= PICTURE_H)
		return 0xFF;

	y = PICTURE_H - y - 1;

	unsigned short rgb = *(pixels + (y * PICTURE_W - 1) + x);
	uint8_t col[3];

	unsigned char red = (rgb >> 11) & 0x1F;
	unsigned char green = (rgb >> 5) & 0x3F;
	unsigned char blue = rgb & 0x1F;

	red = (red << 3) | (red >> 2);
	green = (green << 2) | (green >> 4);
	blue = (blue << 3) | (blue >> 2);
	col[0] = red;
	col[1] = green;
	col[2] = blue;

	return (uint8_t)col[color];
}

void int_to_string(int number, char *str)
{
	sprintf(str, "%d", number);
}

void printcart_init()
{

	nozdata_queue = xQueueCreate(1, PRINTCART_NOZDATA_SZ);

	i2s_parallel_config_t i2scfg = {
		.gpio_bus = {
			PIN_NUM_CART_D1,	 
			PIN_NUM_CART_D2,	 
			PIN_NUM_CART_D3,	 
			PIN_NUM_CART_CSYNC,  
			PIN_NUM_CART_S2,	 
			PIN_NUM_CART_S4,	 
			PIN_NUM_CART_S1,	 
			PIN_NUM_CART_S5,	 
			PIN_NUM_CART_DCLK,	 
			PIN_NUM_CART_S3,	 
			PIN_NUM_CART_F3,	 
			PIN_NUM_CART_F5,	 
			-1, -1, -1, -1		 
		},
		.bits = I2S_PARALLEL_BITS_16,
		.clkspeed_hz = 3333333,  
		.bufsz = WAVEFORM_DMALEN * sizeof(uint16_t),
		.refill_cb = printcart_buffer_filler_fn,
		.refill_cb_arg = nozdata_queue};

	setup(&i2scfg);  
	i2s_parallel_start();
#ifdef CART_IS_COLOR
	printcart_select_waveform(PRINTCART_WAVEFORM_COLOR_B);
#endif
#ifdef CART_IS_BLACK
	printcart_select_waveform(PRINTCART_WAVEFORM_BLACK_B);
#endif

#ifdef CART_IS_COLOR
	printf("Printcart color driver inited3 \n");
#endif
#ifdef CART_IS_BLACK
	printf("Printcart driver inited\n");
#endif
}

#define CMY_ROW_OFFSET 16

uint64_t lcg_state = 0;

void lcg_seed2(uint32_t seed)
{
	lcg_state = seed;
}

uint32_t lcg_rand2()
{

	const uint32_t a = 1103515245;  
	const uint32_t c = 12345;
	const uint32_t m = 32767 * 10;
	lcg_state = (uint32_t)(a * lcg_state + c) % m;

	return lcg_state;
}

extern int *firenozzle;
extern int *cacheIndex;
extern long int *cacheCount;
extern int *reversefire;
extern char *imageloc;

void rev_send_image_row_color(int pos, int *ptr)
{
	uint8_t nozdata[PRINTCART_NOZDATA_SZ];
	memset(nozdata, 0, PRINTCART_NOZDATA_SZ);

	for (int c = 2; c >= 0; c--)
	{
		for (int y = 0; y < 77; y++)
		{

			int v = 0xFF;

			v = get_bgnd_pixel(pos - c * CMY_ROW_OFFSET, y * 2, c);

			v = 255 - v;

			uint64_t lcg = lcg_rand2() & 1023;

			if (v > (lcg) + (rand() % 16) - 16)
			{
				printcart_fire_nozzle_color(nozdata, y + 14, c);
			}

			*ptr += 1;
		}
	}

	xQueueSend(nozdata_queue, nozdata, portMAX_DELAY);

}

#define BLACK_ROW_OFFSET 10
void send_image_row_black(int pos)
{
	uint8_t nozdata[PRINTCART_NOZDATA_SZ];
	memset(nozdata, 0, PRINTCART_NOZDATA_SZ);
	for (int row = 0; row < 2; row++)
	{
		for (int y = 0; y < 168; y++)
		{

			if (get_bgnd_pixel(pos - row * BLACK_ROW_OFFSET, y, 0) != 0xff ||
				get_bgnd_pixel(pos - row * BLACK_ROW_OFFSET, y, 1) != 0xff ||
				get_bgnd_pixel(pos - row * BLACK_ROW_OFFSET, y, 2) != 0xff)
			{

				if (rand() & 1)
				{

					printcart_fire_nozzle_black(nozdata, y, row);
				}
			}
		}
	}

	xQueueSend(nozdata_queue, nozdata, portMAX_DELAY);
}

static const char *TAG = "fs";

extern int *haveimage;
extern int *buffCheck;
extern int *snappy;
#define USE_ACCEL 1

int printstart = 0;

char *joinstring(char *str1, char *str2)
{
	char *result = malloc(strlen(str1) + strlen(str2) + 1);
	if (result == NULL)
	{
		printf("Error: could not allocate memory for concatenated string.\n");
		return NULL;
	}
	strcpy(result, str1);
	strcat(result, str2);
	return result;
}

void init_buffer(ImageRingBuffer *ring_buffer)
{
	ring_buffer->head = 0;
	ring_buffer->tail = 0;
	ring_buffer->count = 0;
	ring_buffer->freeze = false;
}

int push_image(ImageRingBuffer *ring_buffer, uint16_t *img)
{
	if (ring_buffer->count == BUFFER_SIZE)
	{  
		return -1;
	}
	ring_buffer->buffer[ring_buffer->head] = img;
	ring_buffer->head = (ring_buffer->head + 1) % BUFFER_SIZE;
	ring_buffer->count++;

	return 0;
}

uint16_t *pop_image(ImageRingBuffer *ring_buffer)
{
	if (ring_buffer->count == 0)
	{  
		return NULL;
	}
	uint16_t *img = ring_buffer->buffer[ring_buffer->tail];
	ring_buffer->tail = (ring_buffer->tail + 1) % BUFFER_SIZE;
	ring_buffer->count--;
	if (ring_buffer->count == 0)
	{  
		ring_buffer->tail = 0;
		ring_buffer->head = 0;
	}
	return img;
}
bool busy = false;

uint16_t *decode_jpg(void)
{

	{
		FILE *f;
		uint16_t *img = NULL;
		const char *file_foo;
		char numstr[12];
		int_to_string(*firenozzle, numstr);
		char *sdpre = "/sdcard/";
		char *sdpre1 = joinstring(sdpre, imageloc);
		char *sd = joinstring(sdpre1, "/");
		char *end = ".jpg";
		char *cat1 = joinstring(sd, numstr);
		char *cat2 = joinstring(cat1, end);

		free(sdpre1);
		free(sd);
		free(cat1);

		file_foo = cat2;

		ESP_LOGI(TAG, "Reading jpg file %s", file_foo);
		f = fopen(file_foo, "rb");
		if (f == NULL)
		{
			ESP_LOGE(TAG, "Failed to open file for reading");
			free(cat2);  
			busy = false;
			thereIsFuckenNothing = true;
			*haveimage = 0;

			return NULL;
		}

		fseek(f, 0, SEEK_END);	   
		long filesize = ftell(f);  
		fseek(f, 0, SEEK_SET);	   

		if (filesize == -1 || filesize == 0)
		{
			ESP_LOGE(TAG, "Failed to STAT file for reading");
			fclose(f);
			free(cat2);  
			busy = false;
			thereIsFuckenNothing = true;
			*haveimage = 0;

			return NULL;
		}

		uint8_t *picture_start = malloc(filesize);

		if (!picture_start)
		{
			ESP_LOGE(TAG, "Failed to get a buffer for the image");
			fclose(f);
			free(cat2);  
			busy = false;
			thereIsFuckenNothing = true;
			*haveimage = 0;

			return NULL;
		}

		fread(picture_start, filesize, 1, f);
		uint8_t *picture_end = picture_start + filesize;

		decode_image(&img, picture_start, picture_end, filesize, PICTURE_W, PICTURE_H);
		*firenozzle = *firenozzle + 1;
		fclose(f);

		free(picture_start);
		free(cat2);  

		busy = false;

		return img;
	}
	return NULL;
}

void reset_ring_buffer()
{
	bool canContinue = false;
	do
	{
		canContinue = false;
		pixels = pop_image(&ring_buffer);
		if (pixels != NULL)
		{
			printf("Cleaning\n");
			canContinue = true;
			thereIsFuckenNothing = false;
			free(pixels);
		}
	} while (canContinue);
}
void producer_task(void *arg)
{
	ImageRingBuffer *ring_buffer = (ImageRingBuffer *)arg;

	if (ring_buffer->count > 0 && *firenozzle == 0)
	{
		reset_ring_buffer();
	}
	if (imageloc != NULL && strcmp(imageloc, "") != 0 && !busy && !ring_buffer->freeze && ring_buffer->count < BUFFER_SIZE && (ring_buffer->count >= 0 || *firenozzle == 0))
	{
		busy = true;

		uint16_t *img = decode_jpg();
		if (img == NULL)
		{
			busy = false;

		}
		else if (push_image(ring_buffer, img) == -1)
		{
		}
	}
}

static portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
void getImage()
{

	pixels = pop_image(&ring_buffer);

	printstart = 1;

}

bool reversed = false;
int screenshot = 0;

void mainloop(void *arg)
{
	while (1)
	{

		while (printstart == 0)
		{

			vTaskDelay(1);
			if (*snappy == 1)
			{
				printf("Snappy");
				
			}

			if (*haveimage == 1)
			{

				printf("ALIGN:%d", printHeadAlign);
				uint64_t start_time = esp_timer_get_time();

				getImage();
				*haveimage = 0;
				vTaskDelay(1);
				int64_t end_time = esp_timer_get_time();
				int64_t passed = end_time - start_time;
				int64_t delay = printHeadAlign - passed;

				if (delay >= 0)
				{
					taskENTER_CRITICAL(&myMutex);
					esp_rom_delay_us(delay);
					taskEXIT_CRITICAL(&myMutex);
				}

				if (*reversefire == 1)
				{
					reversed = true;
				}
				else
				{
					reversed = false;
				}

				int pos = 0;
				int value = 0;
				int *ptr = &value;
				if (reversed)
				{
					pos = PICTURE_W * 2;
				}

				while (printstart == 1)
				{
#ifdef CART_IS_COLOR

					if (reversed)
					{
						rev_send_image_row_color(pos / 2, ptr);
					}
					else
					{
						rev_send_image_row_color(pos / 2, ptr);
					}
					*ptr += 1;
#endif
#ifdef CART_IS_BLACK
					if (reversed)
					{
						send_image_row_black(pos / 2);
					}
					else
					{
						send_image_row_black(pos / 2);
					}

#endif

					if (reversed)
					{
						pos -= 1;
					}
					else
					{
						pos += 1;
					}

					if (pos > (PICTURE_W * 2) || pos < 0)
					{
						printstart = 0;
						free(pixels);

						lcg_seed2(12345);
					}
				}
			}

			if (*buffCheck == 1 && printstart == 0 && thereIsFuckenNothing == false)
			{

				producer_task(&ring_buffer);
				*buffCheck = 0;
			}

			if (*buffCheck == 2)
			{
				reset_ring_buffer();
				*buffCheck = 0;
			}
		}
	}
}

int cartmain(void)
{
#define GPIO_OUT_PIN_SEL ((1ULL << GPIO_NUM_1))

	gpio_config_t io_conf = {};

	io_conf.intr_type = GPIO_INTR_DISABLE;

	io_conf.mode = GPIO_MODE_OUTPUT;

	io_conf.pin_bit_mask = GPIO_OUT_PIN_SEL;

	io_conf.pull_down_en = 0;

	io_conf.pull_up_en = 0;

	gpio_config(&io_conf);
	gpio_set_level(GPIO_NUM_1, 1);
	lcg_seed2(12345);

	init_buffer(&ring_buffer);
	printf("VERSION: 3");
	printcart_init();

	xTaskCreatePinnedToCore(mainloop, "mainloop", 1024 * 16, NULL, 7, NULL, 1);

	return 0;
}
