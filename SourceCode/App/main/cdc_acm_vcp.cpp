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
/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "usb/cdc_acm_host.h"
#include "usb/vcp_ch34x.hpp"
#include "usb/vcp_cp210x.hpp"
#include "usb/vcp_ftdi.hpp"
#include "usb/vcp.hpp"
#include "usb/usb_host.h"

#include "string_helper.h"

#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "sd_card_main.h"
#include "sdmmc_cmd.h"

#include "global.h"
#include "globalPrinter.h"

using namespace esp_usb;

#define CONFIG_BAUDRATE (115200)
#define CONFIG_STOP_BITS (0)  
#define CONFIG_PARITY (0)     
#define CONFIG_DATA_BITS (8)

static const char *TAG = "VCP CONFIG";
char command[512];
int printStart = 1;
int numOks = 0;

float printHeadOffsetX = 29.95f;
float printHeadOffsetY = 2.25f;
float *offX = &printHeadOffsetX;
float *offY = &printHeadOffsetY;

int printHeadAlign = 20750;
int *offAlign = &printHeadAlign;

bool goober = false;

char printAbort = 0;
int get_buf = 0;
int *buffCheck = &get_buf;

std::unique_ptr<CdcAcmDevice> vcp = nullptr;

CircularBuffer usbbuffer;

namespace
{
    static SemaphoreHandle_t device_disconnected_sem;

    static void addToBuff(uint8_t *data, size_t data_len)
    {
        if (xSemaphoreTake(usbbuffer.spaces, portMAX_DELAY) == pdTRUE)
        {

            if (xSemaphoreTake(usbbuffer.mutex, portMAX_DELAY) == pdTRUE)
            {
                memset(usbbuffer.data[usbbuffer.head], 0, MAX_STRING_LENGTH);  

                memcpy(usbbuffer.data[usbbuffer.head], data, data_len);
                usbbuffer.data[usbbuffer.head][data_len] = '\0';  

                usbbuffer.head = (usbbuffer.head + 1) % DATA_BUFFER_SIZE;

                if (usbbuffer.count < DATA_BUFFER_SIZE)
                {
                    usbbuffer.count++;
                }
                else
                {
                    usbbuffer.tail = (usbbuffer.tail + 1) % DATA_BUFFER_SIZE;
                }

                xSemaphoreGive(usbbuffer.mutex);

                xSemaphoreGive(usbbuffer.items);
            }
        }
    }

    char *stringIndex(char *src, char *search)
    {
        char *p1 = strstr(src, search);
        if (p1 == NULL)
            return NULL;
        return p1;
    }

    int stringPos(const char *line, const char *search)
    {
        char *pos = strstr(line, search);
        if (pos)
        {
            return pos - line;
        }
        return -1;
    }
    static bool handle_rx(const uint8_t *temp2, size_t data_len, void *arg)
    {

         char *data = (char *)malloc(data_len + 1);  
         if (data != NULL)
                    {
             memcpy(data, temp2, data_len);
             data[data_len] = '\0';

                    }

        if (stringIndex((char *)data, "Print Aborted"))
                    {

            printf("Print cancelled\n");
                        printAbort = 1;
                        get_buf = 2;
                    }

        if (data_len < 2048)
        {
            bool canClear = false;

            printf("len:%d, Temp Data: %s", data_len, data );

            memcpy(command, data, data_len+1);

            if (strchr(command, '\n') != NULL)
            { 
                canClear = true;
                }

            if (isOk(command) && canClear)
            {
                memset(command, 0, sizeof(command));

                printStart = 1;
                numOks--;
            }

            if (canClear)
            {
                memset(command, 0, sizeof(command));
            }
        }

        if (printStart == 1 || data_len == 0 || data == NULL || printed == 1 || inkSplurt)
        {

            if (data != NULL) {
                free(data);
            }
            return 1;
        }

        if (data != NULL) {
            free(data);
    }
        return 0;
    }

    static void handle_event(const cdc_acm_host_dev_event_data_t *event, void *user_ctx)
    {
        static std::vector<uint8_t> last_message;  
        static int reconnect_attempts = 0;         
        const int max_reconnect_attempts = 3;      
        switch (event->type)
        {
        case CDC_ACM_HOST_ERROR:
            ESP_LOGE(TAG, "CDC-ACM error has occurred, err_no = %d", event->data.error);
            break;
        case CDC_ACM_HOST_DEVICE_DISCONNECTED:
            ESP_LOGI(TAG, "Device suddenly disconnected");
            vcp.reset();

            xSemaphoreGive(device_disconnected_sem);
            reconnect_attempts = 0;  
            break;
        case CDC_ACM_HOST_SERIAL_STATE:
            ESP_LOGI(TAG, "Serial state notif 0x%04X", event->data.serial_state.val);
            while (reconnect_attempts < max_reconnect_attempts)
            {
                reconnect_attempts++;
                ESP_LOGI(TAG, "Reconnection attempt %d/%d...", reconnect_attempts, max_reconnect_attempts);

                const cdc_acm_host_device_config_t dev_config = {
                    .connection_timeout_ms = 5000,
                    .out_buffer_size = 128,
                    .in_buffer_size = 16 * 1024,
                    .event_cb = handle_event,
                    .data_cb = handle_rx,
                    .user_arg = NULL,
                };

                vcp = std::unique_ptr<CdcAcmDevice>(VCP::open(&dev_config));

                if (vcp != nullptr)
                {
                    ESP_LOGI(TAG, "Reconnected successfully");

                    cdc_acm_line_coding_t line_coding = {
                        .dwDTERate = CONFIG_BAUDRATE,
                        .bCharFormat = CONFIG_STOP_BITS,
                        .bParityType = CONFIG_PARITY,
                        .bDataBits = CONFIG_DATA_BITS,
                    };
                    ESP_ERROR_CHECK(vcp->line_coding_set(&line_coding));

                    if (!last_message.empty())
                    {
                        ESP_LOGI(TAG, "Retrying last message");

                        esp_err_t err = vcp->tx_blocking(last_message.data(), last_message.size());
                        if (err != ESP_OK)
                        {
                            ESP_LOGE(TAG, "Failed to resend message: %d", err);
                        }
                        else
                        {
                            ESP_LOGI(TAG, "Message resent successfully");
                            ESP_ERROR_CHECK(vcp->set_control_line_state(true, true));  
                        }
                    }

                    reconnect_attempts = 0;  
                    return;
                }
                else
                {
                    ESP_LOGW(TAG, "Reconnection attempt %d failed", reconnect_attempts);
                }

                vTaskDelay(pdMS_TO_TICKS(1000));  
            }

            ESP_LOGE(TAG, "Failed to reconnect after %d attempts. Restarting...", max_reconnect_attempts);
            esp_restart();  
            break;

        case CDC_ACM_HOST_NETWORK_CONNECTION:
        default:
            break;
        }
    }

    static void usb_lib_task(void *arg)
    {
        while (1)
        {

            uint32_t event_flags;
            usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
            if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
            {
                ESP_ERROR_CHECK(usb_host_device_free_all());
            }
            if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)
            {
                ESP_LOGI(TAG, "USB: All devices freed");

            }
        }
    }
}

char *joinstring(char *str1, char *str2)
{

    char *result = (char *)malloc(strlen(str1) + strlen(str2) + 1);
    if (result == NULL)
    {
        printf("Error: could not allocate memory for concatenated string.\n");
        return NULL;
    }
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

extern "C" void sendCommand(uint8_t data[])
{

    if (vcp == NULL)
{
        ESP_LOGI(TAG, "boroken");
    }
    if (vcp != NULL)
    {

    printStart = 0;
        ESP_ERROR_CHECK(vcp->tx_blocking(data, strlen((char *)&data[0]),1000));
        ESP_ERROR_CHECK(vcp->set_control_line_state(true, true));

    }
}

#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define MOUNT_POINT "/sdcard"

int get_image = 0;
int *haveimage = &get_image;

int get_snappy = 0;
int *snappy = &get_snappy;

int fire_nozzle = 0;
int *firenozzle = &fire_nozzle;

int reverse_fire = 0;
int *reversefire = &reverse_fire;

char image_loc[100];
char *imageloc = &image_loc[0];

int inkSplurt;

int swapindex = 0;

void stringReplaceFloat(char *str, const char *substring, float value)
{

    if (!str || !substring)
        return;

    char *pos = strstr(str, substring);
    while (pos)
    {  
        char floatStr[20];
        sprintf(floatStr, "%.3f", value);

        size_t lenSubstring = strlen(substring);
        size_t lenFloatStr = strlen(floatStr);
        size_t lenRemaining = strlen(pos) - lenSubstring;

        if (lenFloatStr > lenSubstring)
        {
            memmove(pos + lenFloatStr, pos + lenSubstring, lenRemaining + 1);  
        }

        else if (lenFloatStr < lenSubstring)
        {
            memmove(pos + lenFloatStr, pos + lenSubstring, lenRemaining + 1);  
        }

        strncpy(pos, floatStr, lenFloatStr);

        pos = strstr(pos + lenFloatStr, substring);
    }
}

void resetPrint()
{
    printf("Reset\n");
    inkSplurt = 0;
    fire_nozzle = 0;

    get_buf = 2;
    goober = false;
    printed = 0;
    printAbort = 0;
    printStart = 1;
    numOks = 0;
        esp_restart();
}

extern "C" void printFile(char *filename)
{
    ESP_LOGI("ALOOC", "Free SPIRAM START: %d", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    if (!sdInserted())
    {
        ESP_LOGE("PRINT", "SD Card not detected.\n");
        return;
    }

    FILE *f;

    fire_nozzle = 0;
    char *mount = "/sdcard/\0";
    char *ext = ".gcode";
    char *cat1 = joinstring(mount, filename);
    char *file_foo = joinstring(cat1, ext);
    free(cat1);
    strcpy(image_loc, filename);

    ESP_LOGI(TAG, "Reading file %s", file_foo);
    f = NULL;

    {
        f = fopen(file_foo, "r");
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Failed to open file for reading");
            free(file_foo);

            return;
        }

    }

    free(file_foo);
    char line[100];

    strcpy(command, "oK\n");

    int ink_index = 0;

    get_buf = 1;
    goober = false;
    int lastX = -1;
    int lastY = -1;

    float minX = 99999999999;
    float minY = 99999999999;

    int withinOuterWallSection = 0;
    int lineno = 0;
    while (fgets(line, sizeof(line), f) != NULL)
    {
        if (strstr(line, ";TYPE:Outer wall") != NULL)
        {
            withinOuterWallSection = 1;
        }
        else if (strstr(line, ";TYPE:Bottom surface") != NULL || strstr(line, ";TYPE:Top surface") != NULL || strstr(line, "; stop printing") != NULL)
        {
            withinOuterWallSection = 0;
        }

        if ((withinOuterWallSection) && (strstr(line, "G1 ") != NULL) && (!isComment(line)))
        {
            float x, y;
            int xIndex = stringPos(line, "X");
            int yIndex = stringPos(line, "Y");

            if (xIndex != -1)
            {
                sscanf(line + xIndex + 1, "%f", &x);
                if (x < minX)
                {
                    minX = x;
                }
            }

            if (yIndex != -1)
            {
                sscanf(line + yIndex + 1, "%f", &y);
                if (y < minY)
                {
                    minY = y;
                }
            }

            if (minX == 5.000f)
            {
                printf("Line Number: %d\n", lineno);
                printf("Line: %s\n", line);
                break;
            }
        }
        lineno++;
    }

    printf("X position: %.3f\n", minX);
    printf("Y position: %.3f\n", minY);
    if (minX > 9999999 || minY > 9999999 || minX < 0 || minY < 0)
    {
        minX = 0;
        minY = 0;
    }

    rewind(f);
    while (fgets(line, sizeof(line), f) != NULL)
    {

        {

            strcpy(command, caps(command));
            do
            {
                vTaskDelay(1);

            } while (printStart == 0);
            int skip = false;

            if (printStart == 1)
            {

                if (stringIndex(line, "@snap"))
                {

                    get_snappy = 1;

                    while (get_snappy == 1)
                    {
                        vTaskDelay(1);
                    }
                    skip = true;
                }

                if (stringIndex(line, ";normal"))
                {
                    reverse_fire = 0;
                }
                if (stringIndex(line, ";reversed"))
                {
                    reverse_fire = 1;
                }
                int xIndex = stringPos(line, "X");
                int yIndex = stringPos(line, "Y");

                if (xIndex != -1)
                {
                    sscanf(line + xIndex + 1, "%d", &lastX);
                }

                if (yIndex != -1)
                {
                    sscanf(line + yIndex + 1, "%d", &lastY);
                }

                if (stringIndex(line, ";@INK_START"))
                {

                    ESP_LOGI("ALOOC", "Free SPIRAM START: %d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
                    printf("X: %f Y: %f", *offX, *offY);
                    goober = true;
                    ink_index = 0;
                    swapindex = 0;
                }
                if (stringIndex(line, ";@INK_END"))
                {
                    ESP_LOGI("ALOOC", "Free SPIRAM END: %d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
                    goober = false;
                    ink_index = 0;
                }
                if (goober == false)
                {
                    get_buf = 1;
                }
                else
                {

                }

                bool sx = (stringIndex(line, "@STARTX@"));
                bool ex = (stringIndex(line, "@ENDY@"));
                if ((sx || ex) && goober)
                {
                    float startX = minX + (6.04f * ink_index) + *offX;
                    float startY = 0;
                    float endY = 0;
                    if (swapindex == 0)
                    {
                        reverse_fire = 0;
                        startY = minY - *offY;
                        endY = (minY + 100) - *offY;
                        if (ex)
                        {
                            swapindex = 1;
                        }
                    }
                    else
                    {
                        reverse_fire = 1;
                        startY = (minY + 100) - *offY;
                        endY = minY - *offY;
                        if (ex)
                        {
                            swapindex = 0;
                        }
                    }
                    stringReplaceFloat(line, "@STARTX@\0", startX);
                    stringReplaceFloat(line, "@STARTY@\0", startY);
                    stringReplaceFloat(line, "@ENDY@\0", endY);

                    if (ex)
                    {

                        ink_index += 1;
                    }
                }

                if (stringIndex(line, "@nothinghere@"))
                {
                    ink_index += 1;
                    skip = true;
                }
                if (stringIndex(line, "M73 P"))
                {
                    addToBuff((uint8_t *)line, strlen(line));
                }

                if (stringIndex(line, "; EXECUTABLE_BLOCK_END"))
                {
                    printf("Exec Stop\n");
                    addToBuff((uint8_t *)"Exec Stop\n", strlen("Exec Stop\n"));
                    break;
                }

                if (printAbort == 1)
                {
                    break;
                }

                if ((!isComment(line)) && (skip == false))
                {
                    printStart = 0;

                    if (stringIndex(line, "@Pause"))
                    {

                        get_image = 1;

                        printf("Printing");
                        printStart = 1;

                        strcpy(line, "\0");
                    }

                    if (printStart == 0)
                    {

                        uint8_t commandToSend[512];  
                        snprintf((char *)commandToSend, sizeof(commandToSend)+1, "%s\r", line);

                        sendCommand(commandToSend);
                        numOks++;
                        if (numOks < 20 && goober == false  )
                        {

                            printStart = 1;
                        }
                        else
                        {

                        }
                    }
                    else
                    {
                    }
                }
            }

            strcpy(line, "\0");

        }
    };
    if (feof(f))
    {

        inkSplurt = 0;

        ESP_LOGI(TAG, "GCODE END");

    }
    else
    {

        ESP_LOGE(TAG, "GCODE EXIT BEFORE END");

    }
    fclose(f);
    resetPrint();

    ESP_LOGI("ALOOC", "Free SPIRAM END: %d", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void hufter(void *arg)
{

    device_disconnected_sem = xSemaphoreCreateBinary();
    assert(device_disconnected_sem);

    while (true)
    {
        if (vcp == nullptr)
        {
            vTaskDelay(1);
            const cdc_acm_host_device_config_t dev_config = {
                .connection_timeout_ms = 5000,  
                .out_buffer_size = 128,
                .in_buffer_size = 16 * 1024,
                .event_cb = handle_event,
                .data_cb = handle_rx,
                .user_arg = NULL,
            };

            ESP_LOGI(TAG, "Detecting Printer...");
            vcp = std::unique_ptr<CdcAcmDevice>(VCP::open(&dev_config));

            if (vcp == nullptr)
            {
                ESP_LOGI(TAG, "Failed to connect to printer");
                continue;
            }
            vTaskDelay(10);

            ESP_LOGI(TAG, "Setting up printer");
            cdc_acm_line_coding_t line_coding = {
                .dwDTERate = CONFIG_BAUDRATE,
                .bCharFormat = CONFIG_STOP_BITS,
                .bParityType = CONFIG_PARITY,
                .bDataBits = CONFIG_DATA_BITS,
            };
            ESP_ERROR_CHECK(vcp->line_coding_set(&line_coding));

            xSemaphoreTake(device_disconnected_sem, portMAX_DELAY);
        }
        vTaskDelay(1);
    }
}

extern "C" void init_usb(void)
{

    inkSplurt = 0;

    ESP_LOGI(TAG, "Installing USB Host");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,

    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));

    BaseType_t task_created = xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 10, NULL);
    assert(task_created == pdTRUE);

    ESP_LOGI(TAG, "Installing CDC-ACM driver");
    ESP_ERROR_CHECK(cdc_acm_host_install(NULL));

    VCP::register_driver<FT23x>();
    VCP::register_driver<CP210x>();
    VCP::register_driver<CH34x>();

    BaseType_t task_created2 = xTaskCreatePinnedToCore(hufter, "hufter", 4096 * 2, NULL, 6, NULL, 1);
    assert(task_created2 == pdTRUE);
}
