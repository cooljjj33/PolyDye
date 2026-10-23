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
/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_crc.h"
#include "led_strip.h"
#include "globalPrinter.h"

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

#if CONFIG_IDF_TARGET_ESP32

#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK 14
#define PIN_NUM_CS 13

#elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2

#define PIN_NUM_MISO 7
#define PIN_NUM_MOSI 16
#define PIN_NUM_CLK 15
#define PIN_NUM_CS 6

#elif CONFIG_IDF_TARGET_ESP32C3
#define PIN_NUM_MISO 6
#define PIN_NUM_MOSI 4
#define PIN_NUM_CLK 5
#define PIN_NUM_CS 1

#endif  

#if CONFIG_IDF_TARGET_ESP32S2
#define SPI_DMA_CHAN host.slot
#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#else
#define SPI_DMA_CHAN 1
#endif
#define SD_CARD_DETECT_PIN GPIO_NUM_39

void cpySubstring(char *src, char *dest, char *start, char *end)
{
    char *p1 = strstr(src, start);
    if (p1 == NULL)
        return;
    p1 += strlen(start);
    char *p2 = strstr(p1, end);
    if (p2 == NULL)
        return;
    int len = p2 - p1;
    strncpy(dest, p1, len);
    dest[len] = '\0';
}

char *strIndex(char *src, char *search)
{
    char *p1 = strstr(src, search);
    if (p1 == NULL)
        return NULL;
    return p1;
}

sdmmc_card_t *card;

#define MOUNT_POINT "/sdcard"

void reconnect_sd_card()
{
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {

        .format_if_mount_failed = false,

        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");
}

static sdmmc_host_t host = SDMMC_HOST_DEFAULT();

esp_err_t mountCard()
{
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = false,
#else
        .format_if_mount_failed = false,
#endif  
        .max_files = 5,
        .allocation_unit_size = 32 * 1024};

   host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; 
    static sdmmc_card_t *card;

    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    ESP_LOGI(TAG, "Using SDMMC peripheral");

        slot_config.width = 4;
             slot_config.clk = 15;
            slot_config.cmd = 16;
            slot_config.d0 = 6;

            slot_config.d1 = 7;
            slot_config.d2 = 4;
            slot_config.d3 = 5;
    ESP_LOGI(TAG, "Mounting filesystem");
        int card_present = !gpio_get_level(SD_CARD_DETECT_PIN);
        if (card_present)
        {
            ESP_DRAM_LOGI(TAG, "SD Card inserted, trying to mount...");

            ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

            if (ret == ESP_OK)
            {
                ESP_DRAM_LOGI(TAG, "SD Card mounted successfully");
                sdmmc_card_print_info(stdout, card);

            }
            else
            {
                ESP_DRAM_LOGE(TAG, "Failed to mount SD Card");
            }
        }

    else
    {
        ESP_DRAM_LOGI(TAG, "SD Card removed, trying to unmount...");

        ret = esp_vfs_fat_sdcard_unmount(mount_point, card);
        card = NULL;
        if (ret == ESP_OK)
        {
            ESP_DRAM_LOGI(TAG, "SD Card unmounted successfully");
        }
        else
        {
            ESP_DRAM_LOGE(TAG, "Failed to unmount SD Card: %s", esp_err_to_name(ret));
        }
        ESP_DRAM_LOGI(TAG, "SD Card unmounted");
    }

    return ESP_OK;
}

void setup_sd_card_detect()
{
    gpio_config_t io_conf = {

        .pin_bit_mask = (1ULL << SD_CARD_DETECT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1};
    gpio_config(&io_conf);

}

#define FIRMWARE_PATH "/sdcard/firmware.bin"
#define WWW_PATH "/sdcard/www.bin"

#define FIRMWARE_PATH_R "/sdcard/firmware.cur"
#define WWW_PATH_R "/sdcard/www.cur"

void read_file_from_sd(const char *file_path, uint8_t **file_data, size_t *file_size)
{
    FILE *f = fopen(file_path, "r");
    if (f == NULL)
    {
        ESP_LOGE("SDCard", "Failed to open file: %s", file_path);
        return;
    }

    fseek(f, 0, SEEK_END);
    *file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    *file_data = (uint8_t *)malloc(*file_size);
    if (*file_data == NULL)
    {
        ESP_LOGE("SDCard", "Failed to allocate memory for file");
        fclose(f);
        return;
    }

    fread(*file_data, 1, *file_size, f);
    fclose(f);

    ESP_LOGI("SDCard", "File read successfully from SD card");
}

void flash_binary_ai(const char *partition_label, uint8_t *file_data, size_t file_size, int partition_type)
{

    const esp_partition_t *current_partition = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Current firmware version: %s", current_partition->label);

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition)
    {
        ESP_LOGE(TAG, "Failed to get OTA partition");
        return;
    }

    if (strcmp(update_partition->label, current_partition->label) == 0)
    {
        ESP_LOGE(TAG, "Update partition is the same as the current running partition");
        return;
    }

    ESP_LOGI(TAG, "Updating firmware to version: %s", update_partition->label);

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start OTA (%s)", esp_err_to_name(err));
        return;
    }

    err = esp_ota_write(ota_handle, file_data, file_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write OTA data (%s)", esp_err_to_name(err));
        esp_ota_end(ota_handle);
        return;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to finish OTA (%s)", esp_err_to_name(err));
        return;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set OTA boot partition (%s)", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Firmware update complete, restarting system...");

}

void flash_binary_www(const char *partition_label, uint8_t *file_data, size_t file_size, int partition_type)
{

    const esp_partition_t *partition = esp_partition_find_first(
        partition_type, ESP_PARTITION_SUBTYPE_ANY, partition_label);

    if (!partition)
    {
        ESP_LOGE(TAG, "Failed to find partition with label: %s", partition_label);
        return;
    }

    ESP_LOGI(TAG, "Found partition '%s', erasing...", partition_label);

    esp_err_t err = esp_partition_erase_range(partition, 0, partition->size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to erase partition: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Partition '%s' erased, writing data...", partition_label);

    err = esp_partition_write(partition, 0, file_data, file_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write data to partition: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Partition '%s' updated successfully.", partition_label);
}

void checkFirmwareUpdate()
{

    const gpio_num_t LED_GPIO = GPIO_NUM_38;  

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(LED_GPIO, 0);

        const gpio_num_t LED_GPIO_E = GPIO_NUM_40;  

    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << LED_GPIO_E),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf2);

    gpio_set_level(LED_GPIO_E, 0);

    uint8_t *firmware_data = NULL;
    size_t firmware_size = 0;
    read_file_from_sd(FIRMWARE_PATH, &firmware_data, &firmware_size);

    uint8_t *www_data = NULL;
    size_t www_size = 0;
    read_file_from_sd(WWW_PATH, &www_data, &www_size);
    bool reboot = false;

    if (firmware_data != NULL)
    {

        if (rename(FIRMWARE_PATH, FIRMWARE_PATH_R) != 0)
        {
            gpio_set_level(LED_GPIO_E, 1);

            ESP_LOGE(TAG, "Rename failed");

            return;
        }
        gpio_set_level(LED_GPIO, 1);

        flash_binary_ai("ota_0", firmware_data, firmware_size, ESP_PARTITION_TYPE_APP);
        free(firmware_data);
        reboot = true;
    }

    if (www_data != NULL)
    {
        if (rename(WWW_PATH, WWW_PATH_R) != 0)
        {
            gpio_set_level(LED_GPIO_E, 1);

            ESP_LOGE(TAG, "Rename failed");

            return;
        }
        gpio_set_level(LED_GPIO, 1);

        flash_binary_www("www", www_data, www_size, ESP_PARTITION_TYPE_DATA);
        free(www_data);
        reboot = true;
    }

    if (reboot)
    {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI("Flash", "Firmware written successfully, rebooting...");

        esp_restart();
    }
    else
    {

    }
}

void checkWifiDetails()
{
    char line[100];
    FILE *f;
    const char *sd_wifi_details = MOUNT_POINT "/wifi.txt";
    const char *sd_wifi_details_cur = MOUNT_POINT "/wifi.cur";
    struct stat st;
    if (stat(sd_wifi_details, &st) == 0)
    {
        f = fopen(sd_wifi_details, "r");
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Failed to open file for reading");
            return;
        }
        char *password = malloc(100);
        char *ssid = malloc(100);
        while (fgets(line, sizeof(line), f) != NULL)
        {
            if (strIndex(line, "ssid=") != NULL)
            {
                cpySubstring(line, ssid, "ssid=\"", "\"");
            }

            if (strIndex(line, "password=") != NULL)
            {
                cpySubstring(line, password, "password=\"", "\"");
            }
        }
        fclose(f);

        printf("Renaming files");
        if (rename(sd_wifi_details, sd_wifi_details_cur) != 0)
        {
            ESP_LOGE(TAG, "Rename failed");
            return;
        }

        ESP_LOGI(TAG, "ESP_WIFI_CONNECT");
        nvs_handle_t nvs_handle;
        nvs_open("phantom_app", NVS_READWRITE, &nvs_handle);


        nvs_set_str(nvs_handle, "wifi_ssid", ssid);
        nvs_set_str(nvs_handle, "wifi_password", password);
        nvs_commit(nvs_handle);
        free(ssid);
        free(password);
    }
}

void sd_card_task(void *arg)
{
    bool card_present = false;
    bool prev_card_status = false;
    int debounce_delay = 200 / portTICK_PERIOD_MS;  
    int debounce_count = 0;

    while (1)
    {
        card_present = !gpio_get_level(SD_CARD_DETECT_PIN);

        if (card_present != prev_card_status)
        {
            if (debounce_count > 0)
            {
                debounce_count--;
            }
            else
            {
                debounce_count = debounce_delay;
                if (card_present)
                {

                    ESP_LOGI(TAG, "SD Card inserted, trying to mount...");

                    mountCard();
                    checkWifiDetails();
                    checkFirmwareUpdate();

                }
                else
                {

                    ESP_LOGI(TAG, "SD Card removed, unmounting...");
                    mountCard();

                }
                prev_card_status = card_present;
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);  
    }
}

void sd_main(int skipWifi)
{

    {
        setup_sd_card_detect();
    }

    xTaskCreatePinnedToCore(sd_card_task, "sd_card_task", 1024 * 8, NULL, 1, NULL, 1);
}
