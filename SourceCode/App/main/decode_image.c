/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0 AND CC0-1.0
 * 
 * CMY Labs PTY LTD's original copyrightable additions are NCL-1.0 ONLY.
 * Retained upstream expression remains under CC0-1.0.
 * The licenses apply to their respective contributions, not as alternatives.
 * Retained upstream license identifiers below describe upstream material.
 * Modified in this project; upstream notices are preserved.
 * Upstream: Espressif Systems (Shanghai) CO LTD; CC0 / Public Domain.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
#include "decode_image.h"
#include "jpeg_decoder.h"
#include "esp_log.h"
#include "esp_check.h"
#include <string.h>
#include "freertos/FreeRTOS.h"

const char *TAG = "ImageDec";

esp_err_t decode_image(uint16_t **pixels, uint8_t * image_jpg_start, uint8_t * image_jpg_end, long filesize, int w, int h)
{
    *pixels = NULL;
    esp_err_t ret = ESP_OK;

        *pixels = heap_caps_calloc(h * w, sizeof(uint16_t), MALLOC_CAP_SPIRAM);

    ESP_GOTO_ON_FALSE((*pixels), ESP_ERR_NO_MEM, err, TAG, "Error allocating memory for lines");

    esp_jpeg_image_cfg_t jpeg_cfg = {
        .indata = (uint8_t *)image_jpg_start,
        .indata_size = h * w * sizeof(uint16_t), 
        .outbuf = (uint8_t*)(*pixels),
        .outbuf_size = w * h * sizeof(uint16_t),
        .out_format = JPEG_IMAGE_FORMAT_RGB565,
        .out_scale = JPEG_IMAGE_SCALE_0,
        .flags = {
            .swap_color_bytes = 0,
        }
    };

    esp_jpeg_image_output_t outimg;
    esp_jpeg_decode(&jpeg_cfg, &outimg);

    ESP_LOGI(TAG, "JPEG image decoded! Size of the decoded image is: %dpx x %dpx", outimg.width, outimg.height);

    return ret;
err:

    ESP_LOGI(TAG, "You dont have enough memory");
    if (*pixels != NULL) {
        free(*pixels);
    }
    return ret;
}
