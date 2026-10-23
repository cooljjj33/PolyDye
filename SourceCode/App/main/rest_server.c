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
/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_wifi.h"
#include "miniz.h"

#include "lwip/sockets.h"
#include <esp_http_server.h>

#include "sdkconfig.h"

#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "command_handler.h"
#include "cdc_acm_vcp_main.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "sd_card_main.h"
#include "sdmmc_cmd.h"
#include "rest_server.h"

#include <mbedtls/base64.h>
#include "global.h"
#include "globalPrinter.h"

int printed;
char tempFileName[250];
char fileDetailsFilename[250];

static const char *REST_TAG = "esp-rest";
static const char *TAG = "ws_echo_server";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)

#define SCRATCH_BUFSIZE (768)

extern char *usbmessage;
extern float *offX;
extern float *offY;
extern int *offAlign;
extern int *buffCheck;
printer_settings_t *global_printer_settings;

bool file_ready_for_extraction = false;
char zip_file_path[256];  

typedef struct rest_server_context
{
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

char *stringIndex(char *src, char *search)
{
    char *p1 = strstr(src, search);
    if (p1 == NULL)
        return NULL;
    return p1;
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html"))
    {
        type = "text/html";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".js"))
    {
        type = "application/javascript";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".css"))
    {
        type = "text/css";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".png"))
    {
        type = "image/png";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".ico"))
    {
        type = "image/x-icon";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".svg"))
    {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

typedef struct ws_client_info
{
    int fd;
    uint32_t client_id;
    struct ws_client_info *next;
} ws_client_info_t;

static ws_client_info_t *g_ws_clients = NULL;

static uint32_t client_id_counter = 0;
static SemaphoreHandle_t client_list_mutex;

void init_ws_server()
{
    client_list_mutex = xSemaphoreCreateMutex();
    if (client_list_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex for client list");
    }
}
esp_err_t send_message_to_all_ws_clients(const char *message)
{

    if (server == NULL || !g_ws_clients)
    {

        return ESP_ERR_INVALID_STATE;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)message;
    ws_pkt.len = strlen(message);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    xSemaphoreTake(client_list_mutex, portMAX_DELAY);

    ws_client_info_t *client = g_ws_clients;
    ws_client_info_t *prev_client = NULL;

    while (client)
    {
        esp_err_t ret = httpd_ws_send_frame_async(server, client->fd, &ws_pkt);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to send message to client fd %d (client_id ). Removing it.", client->fd);

            ws_client_info_t *temp = client;
            client = client->next;

            if (prev_client)
            {
                prev_client->next = client;
            }
            else
            {
                g_ws_clients = client;
            }
            free(temp);
        }
        else
        {
            prev_client = client;
            client = client->next;
        }

    }

    xSemaphoreGive(client_list_mutex);

    return ESP_OK;
}

void remove_ws_client(int fd)
{
    xSemaphoreTake(client_list_mutex, portMAX_DELAY);

    ws_client_info_t *client = g_ws_clients;
    ws_client_info_t *prev_client = NULL;

    while (client)
    {
        if (client->fd == fd)
        {
            if (prev_client)
            {
                prev_client->next = client->next;
            }
            else
            {
                g_ws_clients = client->next;
            }

            free(client);
            break;
        }
        prev_client = client;
        client = client->next;
    }

    xSemaphoreGive(client_list_mutex);
}

void add_ws_client(int fd)
{
    xSemaphoreTake(client_list_mutex, portMAX_DELAY);

    ws_client_info_t *client = g_ws_clients;
    ws_client_info_t *prev_client = NULL;

    while (client)
    {
        if (client->fd == fd)
        {
            ESP_LOGI(TAG, "Removing existing client with fd %d before adding new one", fd);
            if (prev_client)
            {
                prev_client->next = client->next;
            }
            else
            {
                g_ws_clients = client->next;
            }
            free(client);
            break;  
        }
        prev_client = client;
        client = client->next;
    }

    ws_client_info_t *new_client = calloc(1, sizeof(ws_client_info_t));
    if (new_client == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for new client");
        xSemaphoreGive(client_list_mutex);
        return;
    }

    new_client->fd = fd;
    new_client->client_id = client_id_counter++;

    new_client->next = g_ws_clients;
    g_ws_clients = new_client;

    xSemaphoreGive(client_list_mutex);
}

static esp_err_t echo_handler(httpd_req_t *req)
{

    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        add_ws_client(httpd_req_to_sockfd(req));
        server = req->handle;
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;

    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        remove_ws_client(httpd_req_to_sockfd(req));
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    if (ws_pkt.len)
    {

        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;

        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            remove_ws_client(httpd_req_to_sockfd(req));
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }

        sendCommand(ws_pkt.payload);
    }

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK)
    {
        remove_ws_client(httpd_req_to_sockfd(req));
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);

    return ret;
}

static const httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = echo_handler,
    .user_ctx = NULL,
    .is_websocket = true};

static esp_err_t rest_common_get_handler(httpd_req_t *req)
{

    char filepath[FILE_PATH_MAX];
    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;

    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/')
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    }
    else
    {
        strlcat(filepath, req->uri, sizeof(filepath));
    }

    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1)
    {
        ESP_LOGE(REST_TAG, "Failed to open file: %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Authorization, Content-Type, Accept");

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    esp_err_t result = ESP_OK;

    if (strcmp(req->uri, "/favicon.ico") == 0)
    {
        if (fd != -1)
        {
            close(fd);
        }

        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Favicon not found");
        return ESP_FAIL;
    }
    do
    {
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1)
        {
            ESP_LOGE(REST_TAG, "Failed to read file: %s", filepath);
            result = ESP_FAIL;
            break;
        }
        if (read_bytes > 0)
        {
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK)
            {
                ESP_LOGE(REST_TAG, "File sending failed for: %s", filepath);
                result = ESP_FAIL;
                break;
            }
        }
    } while (read_bytes > 0);

    close(fd);

    if (result == ESP_FAIL)
    {
        httpd_resp_sendstr_chunk(req, NULL);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
    }

    ESP_LOGI(REST_TAG, "File sending complete: %s", filepath);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t post_handler(httpd_req_t *req)
{

     if (req->method == HTTP_OPTIONS)  
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");

    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "access-control-allow-origin, content-type");
    httpd_resp_send(req, NULL, 0);  
    return ESP_OK;
}   

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");

    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "access-control-allow-origin, content-type");
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;

    if (total_len >= SCRATCH_BUFSIZE)
    {

        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {

            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);

    int postCommand = cJSON_GetObjectItem(root, "command")->valueint;
    cJSON *params = cJSON_GetObjectItem(root, "parameters");

    if (postCommand == Settings)
    {
        while (printstart == 1 || (*buffCheck == 0 && inkSplurt == 1))
        {

        }
        printer_settings_t amprinter_settings;

        amprinter_settings.bedsize_x = cJSON_GetObjectItem(params, "bedsize_x")->valueint;
        amprinter_settings.bedsize_y = cJSON_GetObjectItem(params, "bedsize_y")->valueint;
        amprinter_settings.bedsize_z = cJSON_GetObjectItem(params, "bedsize_z")->valueint;

        strncpy(amprinter_settings.cam_url, cJSON_GetObjectItem(params, "cam_url")->valuestring, 256);
        amprinter_settings.extruder_fr = cJSON_GetObjectItem(params, "extruder_fr")->valueint;
        amprinter_settings.print_accel = cJSON_GetObjectItem(params, "print_accel")->valueint;
        amprinter_settings.travel_accel = cJSON_GetObjectItem(params, "travel_accel")->valueint;
        amprinter_settings.travel_accel_z = cJSON_GetObjectItem(params, "travel_accel_z")->valueint;
        amprinter_settings.travel_fr = cJSON_GetObjectItem(params, "travel_fr")->valueint;
        strncpy(amprinter_settings.coord_x_adj, cJSON_GetObjectItem(params, "coord_x_adj")->valuestring, 32);
        strncpy(amprinter_settings.coord_y_adj, cJSON_GetObjectItem(params, "coord_y_adj")->valuestring, 32);
        strncpy(amprinter_settings.align_adj, cJSON_GetObjectItem(params, "align_adj")->valuestring, 32);

        nvs_handle_t my_handle;
        esp_err_t err;

        err = nvs_open("phantom_app", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {

            printf("nvsOpenERROR");
            return 1;
        }

        size_t required_size = sizeof(printer_settings_t);

        err = nvs_set_blob(my_handle, "pr_settings", &amprinter_settings, required_size);

        printf("%s %d", "SAVED Settings :", required_size);
        if (err != ESP_OK)
        {
            nvs_close(my_handle);

            httpd_resp_sendstr(req, esp_err_to_name(err));
            return ESP_OK;
        }

        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            nvs_close(my_handle);
            httpd_resp_sendstr(req, "Post control ERROR");
            return ESP_OK;
        }

        nvs_close(my_handle);
        httpd_resp_sendstr(req, "Post control value successfully");

        return ESP_OK;
    }

    char str[12];
    sprintf(str, "%d", PrintFile);

    ESP_LOGI(REST_TAG, "File sending complete{%d}{%d}", postCommand, PrintFile);
    if (postCommand == PrintFile)
    {
        postCommand = -1;

        strcpy(tempFileName, cJSON_GetObjectItem(params, "fileName")->valuestring);

        printed = 1;
        inkSplurt = 1;

    }

    if (postCommand == SelectFile)
    {
        strcpy(fileDetailsFilename, cJSON_GetObjectItem(params, "fileName")->valuestring);
    }

    if (postCommand == Move)
    {
        postCommand = -1;
        cJSON *xItem = cJSON_GetObjectItem(params, "x");
        cJSON *yItem = cJSON_GetObjectItem(params, "y");

        if (xItem && cJSON_IsNumber(xItem))
        {
            *offX += (float)xItem->valuedouble;
        }

        if (yItem && cJSON_IsNumber(yItem))
        {
            *offY += (float)yItem->valuedouble;
        }

        sprintf(global_printer_settings->coord_x_adj, "%.2f", *offX);
        sprintf(global_printer_settings->coord_y_adj, "%.2f", *offY);

        nvs_handle_t my_handle;
        esp_err_t err;

        err = nvs_open("phantom_app", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {

            printf("nvsOpenERROR");
            return 1;
        }

        size_t required_size = sizeof(printer_settings_t);

        err = nvs_set_blob(my_handle, "pr_settings", &*global_printer_settings, required_size);

        printf("%s %d", "SAVED SETTINGS :", required_size);
        if (err != ESP_OK)
        {
            nvs_close(my_handle);
        }
        else
        {

            err = nvs_commit(my_handle);
            if (err != ESP_OK)
            {
                nvs_close(my_handle);
            }
        }
        nvs_close(my_handle);
    }

    if (postCommand == Align)
    {
        postCommand = -1;
        printf("al1: %s", global_printer_settings->align_adj);

        cJSON *uItem = cJSON_GetObjectItem(params, "u");

        if (uItem && cJSON_IsNumber(uItem))
        {
            *offAlign += (int)uItem->valueint;
        }

        cJSON *dItem = cJSON_GetObjectItem(params, "d");

        if (dItem && cJSON_IsNumber(dItem))
        {
            *offAlign += (int)dItem->valueint;
        }

        printHeadAlign = *offAlign;

        sprintf(global_printer_settings->align_adj, "%d", *offAlign);

        nvs_handle_t my_handle;
        esp_err_t err;

        err = nvs_open("phantom_app", NVS_READWRITE, &my_handle);
        if (err != ESP_OK)
        {

            printf("nvsOpenERROR");
            return 1;
        }

        size_t required_size = sizeof(printer_settings_t);

        err = nvs_set_blob(my_handle, "pr_settings", &*global_printer_settings, required_size);

        printf("%s %d", "SAVED Settings :", required_size);
        if (err != ESP_OK)
        {
            nvs_close(my_handle);
        }
        else
        {

            err = nvs_commit(my_handle);
            if (err != ESP_OK)
            {
                nvs_close(my_handle);
            }
        }
        nvs_close(my_handle);
    }

    if (postCommand == Login)
    {
        char *ssid = malloc(33);
        memset(ssid, 0, 33);
        char *password = malloc(65);
        memset(password, 0, 65);

        cJSON *param = cJSON_GetObjectItem(root, "parameters");
        cJSON *key_value = cJSON_GetObjectItem(param, "ssid");
        if (key_value != NULL && key_value->type == cJSON_String)
        {
            ssid = key_value->valuestring;
        }
        key_value = cJSON_GetObjectItem(param, "password");
        if (key_value != NULL && key_value->type == cJSON_String)
        {
            password = key_value->valuestring;
        }

        nvs_handle_t nvs_handle;
        nvs_open("phantom_app", NVS_READWRITE, &nvs_handle);
        if (ssid != NULL && strlen(ssid) > 0 && password != NULL && strlen(password) > 0)
        {
            nvs_set_str(nvs_handle, "wifi_ssid", ssid);
            nvs_set_str(nvs_handle, "wifi_password", password);
            nvs_commit(nvs_handle);
        }

    }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");

    return ESP_OK;
}

printer_settings_t *getPrinterSettings()
{

    while (printstart == 1 || (*buffCheck == 0 && inkSplurt == 1))
    {

    }
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open("phantom_app", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("NVS OPEN BAD");
        return NULL;
    }

    size_t required_size = 0;  
    err = nvs_get_blob(my_handle, "pr_settings", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(my_handle);
        printf("NVS READ BAD");
        return NULL;
    }
    printf("REQUIRED SIZE: %d", required_size);

    printer_settings_t *printer_settings = malloc(required_size);
    if (required_size > 0)
    {
        err = nvs_get_blob(my_handle, "pr_settings", printer_settings, &required_size);
        if (err != ESP_OK)
        {
            nvs_close(my_handle);
            printf("ERROR GETTING BLOB");
            free(printer_settings);
            return NULL;
        }
    }

    if (err != ESP_OK)
    {
        nvs_close(my_handle);
        return NULL;
    }

    nvs_close(my_handle);
    return printer_settings;
}

#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define MOUNT_POINT "/sdcard"

printer_dir_command_t *listDir(int offset, int limit)
{

    char entrypath[FILE_PATH_MAX];

    const char *entrytype;

    struct dirent *entry;

    const char *dirpath = "/sdcard";

    printer_dir_command_t *command;
    command = malloc(sizeof(printer_dir_command_t));

    DIR *dir = opendir(dirpath);

    strlcpy(entrypath, dirpath, sizeof(entrypath));

    command->status = STATUS_OK;
    if (!dir)
    {
        command->status = STATUS_DIR_GONE;
        return command;
    }

    int count = 0;
    int processed = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        entrytype = (entry->d_type == DT_DIR ? "directory" : "file");

        if (strcmp(entrytype, "file") == 0 && CHECK_FILE_EXTENSION(entry->d_name, ".gcode"))
        {

            if (processed < offset)
            {
                processed++;
                continue;
            }

            if (count == limit)
            {
                break;
            }

            command->fileList[count] = (sd_file_t *)malloc(sizeof(sd_file_t));
            strncpy(command->fileList[count]->fileName, entry->d_name, sizeof(command->fileList[count]->fileName));

            char filePath[500] = "/sdcard/";
            strcat(filePath, entry->d_name);
            count++;
            processed++;
        }
    }
    closedir(dir);

    command->fileCount = count;

    return command;
}

#define CHUNKSIZE 256

void send_json_data(httpd_req_t *req, const char *json_data, size_t len, bool skipEnd)
{
    size_t offset = 0;
    while (offset < len)
    {
        size_t chunksize = len - offset;
        if (chunksize > CHUNKSIZE)
        {
            chunksize = CHUNKSIZE;
        }
        if (httpd_resp_send_chunk(req, json_data + offset, chunksize) != ESP_OK)
        {
            break;
        }
        offset += chunksize;
    }
    if (!skipEnd)
    {
        httpd_resp_send_chunk(req, NULL, 0);
    }
}

static esp_err_t sd_data_get_handler(httpd_req_t *req)
{

    if (req->method == HTTP_OPTIONS)  
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");

    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "access-control-allow-origin, content-type");
    httpd_resp_send(req, NULL, 0);  
    return ESP_OK;
}

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");

    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "access-control-allow-origin, content-type");

    if (!sdInserted())
    {
        ESP_LOGE("REST11111", "SD Card not detected.\n");
        const char *resp = "{\"resp\": \"SD Card not detected.\",\"status\": \"403\"}";
        httpd_resp_send(req, resp, strlen(resp));
    }
    else
    {

        ESP_LOGI("", "Reading SD Card");

        const int limit = 10;
        int offset = 0;
        bool first_iteration = true;

        httpd_resp_set_type(req, "application/json");

         httpd_resp_send_chunk(req, "{", -1);

        char buffer[256];  

        int len = snprintf(buffer, sizeof(buffer),
                           "\"command\": %d, \"fileList\": [",
                           1  
        );
        httpd_resp_send_chunk(req, buffer, len);

        printer_dir_command_t *command = NULL;

        while (1)
        {
            command = listDir(offset, limit);
            if (command->fileCount == 0)
            {
                break;
            }

            for (int i = 0; i < command->fileCount; i++)
            {
                len = snprintf(buffer, sizeof(buffer),
                               "%s{\"fileName\": \"%s\"}",
                               (!first_iteration || i != 0) ? "," : "",
                               command->fileList[i]->fileName);

                httpd_resp_send_chunk(req, buffer, len);
            }

            offset += command->fileCount;
            first_iteration = false;

            free(command);
        }

        len = snprintf(buffer, sizeof(buffer),
                       "], \"fileCount\": %d, \"status\": \"%s\"",
                       offset, "STATUS_OK");  
        httpd_resp_send_chunk(req, buffer, len);

        httpd_resp_send_chunk(req, "}", -1);

        httpd_resp_send_chunk(req, NULL, 0);

        if (command)
            free(command);
    }
    return ESP_OK;
}

static esp_err_t options_handler(httpd_req_t *req)
{
    printf("I hit options");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Authorization, Content-Type, Accept");
    httpd_resp_send(req, NULL, 0);  
    return ESP_OK;
}

cJSON *populate_jsongetsettings_alt(printer_settings_t *printerSettings)
{
    printer_command_t command;
    enum PrinterCommandType command_type;
    command_type = Settings;
    command.id = command_type;
    command.command = command_type;
    command.parameters = cJSON_CreateObject();

    cJSON_AddItemToObject(command.parameters, "bedsize_x", cJSON_CreateNumber(printerSettings->bedsize_x));
    cJSON_AddItemToObject(command.parameters, "bedsize_y", cJSON_CreateNumber(printerSettings->bedsize_y));
    cJSON_AddItemToObject(command.parameters, "bedsize_z", cJSON_CreateNumber(printerSettings->bedsize_z));
    cJSON_AddItemToObject(command.parameters, "cam_url", cJSON_CreateString(printerSettings->cam_url));
    cJSON_AddItemToObject(command.parameters, "extruder_fr", cJSON_CreateNumber(printerSettings->extruder_fr));
    cJSON_AddItemToObject(command.parameters, "travel_fr", cJSON_CreateNumber(printerSettings->travel_fr));
    cJSON_AddItemToObject(command.parameters, "travel_accel", cJSON_CreateNumber(printerSettings->travel_accel));
    cJSON_AddItemToObject(command.parameters, "travel_accel_z", cJSON_CreateNumber(printerSettings->travel_accel_z));
    cJSON_AddItemToObject(command.parameters, "print_accel", cJSON_CreateNumber(printerSettings->print_accel));
    cJSON_AddItemToObject(command.parameters, "coord_x_adj", cJSON_CreateString(printerSettings->coord_x_adj));
    cJSON_AddItemToObject(command.parameters, "coord_y_adj", cJSON_CreateString(printerSettings->coord_y_adj));
    cJSON_AddItemToObject(command.parameters, "align_adj", cJSON_CreateString(printerSettings->align_adj));

    cJSON *command_json = cJSON_CreateObject();
    if (command_json == NULL)
    {
        printf("Error creating command_json \n");
        cJSON_Delete(command.parameters);
        return NULL;
    }
    cJSON_AddItemToObject(command_json, "id", cJSON_CreateNumber(command.id));
    cJSON_AddItemToObject(command_json, "command", cJSON_CreateNumber(command.command));
    cJSON_AddItemToObject(command_json, "parameters", command.parameters);

    char *command_json_string = cJSON_Print(command_json);
    if (command_json_string == NULL)
    {
        printf("Error creating command_json_string \n");
        cJSON_Delete(command.parameters);
        return NULL;
    }

    return command_json;
}

static esp_err_t extract_thumbnail(const char *filepath, char **thumbnail_data, size_t *thumbnail_size)
{
    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return ESP_FAIL;
    }

    char line[256];
    int in_thumbnail_section = 0;
    size_t total_length = 0;
    size_t allocated_size = 4096;  
    *thumbnail_data = malloc(allocated_size);

    if (!*thumbnail_data)
    {
        fclose(file);
        return ESP_ERR_NO_MEM;
    }
    int i = 0;
    while (fgets(line, sizeof(line), file))
    {
        if (strstr(line, "; thumbnail begin"))
        {
            in_thumbnail_section = 1;
            continue;
        }
        else if (strstr(line, "; thumbnail end"))
        {
            in_thumbnail_section = 0;
            break;
        }

        if (in_thumbnail_section)
        {
            size_t line_length = strlen(line);
            if (total_length + line_length > allocated_size)
            {
                allocated_size *= 2;
                *thumbnail_data = realloc(*thumbnail_data, allocated_size);
                if (!*thumbnail_data)
                {
                    fclose(file);
                    return ESP_ERR_NO_MEM;
                }
            }
            strcpy(*thumbnail_data + total_length, line);
            total_length += line_length;
        }
        i++;
        if (i > 5000)
        {
            break;
        }
    }

    fclose(file);

    if (total_length == 0)
    {
        free(*thumbnail_data);
        *thumbnail_data = NULL;
        *thumbnail_size = 0;
        return ESP_FAIL;
    }

    *thumbnail_size = total_length;

    return ESP_OK;
}

static esp_err_t gcode_thumbnail_get_handler(httpd_req_t *req)
{

    char query[256];
    char filepath[256];
    char *thumbnail_data = NULL;
    size_t thumbnail_size = 0;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    {

        if (httpd_query_key_value(query, "filename", filepath, sizeof(filepath)) == ESP_OK)
        {

        }
        else
        {
            ESP_LOGE(TAG, "Filename not found in query parameters.");
            const char *resp = "{\"resp\": \"Filename not found.\",\"status\": \"400\"}";
            httpd_resp_send(req, resp, strlen(resp));
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "No query string found.");
        const char *resp = "{\"resp\": \"No query string found.\",\"status\": \"400\"}";
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_FAIL;
    }

    if (!sdInserted())
    {
        ESP_LOGE(TAG, "SD Card not detected.");
        const char *resp = "{\"resp\": \"SD Card not detected.\",\"status\": \"403\"}";
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_FAIL;
    }

    char full_filepath[512];
    snprintf(full_filepath, sizeof(full_filepath), "/sdcard/%s", filepath);

    esp_err_t ret = extract_thumbnail(full_filepath, &thumbnail_data, &thumbnail_size);
    if (ret != ESP_OK || !thumbnail_data)
    {
        ESP_LOGE(TAG, "Failed to extract thumbnail.");
        const char *resp = "{\"resp\": \"\",\"status\": \"200\"}";
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_FAIL;
    }

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "resp", thumbnail_data);
    cJSON_AddStringToObject(json, "status", "200");

    const char *json_str = cJSON_Print(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    free(thumbnail_data);
    cJSON_Delete(json);
    free((void *)json_str);

    return ESP_OK;
}

static esp_err_t rest_getdata_get_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");

    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "access-control-allow-origin, content-type");

    printer_settings_t *settings = getPrinterSettings();

    if (settings == NULL)
    {
        printf("SETTINGS ARE BAD");
        return !ESP_OK;
    }
    cJSON *obj = populate_jsongetsettings_alt(settings);

    const char *data = cJSON_Print(obj);

    if (data == NULL)
    {
        ESP_LOGI(REST_TAG, "Error on get data jsson generation");
    }

    send_json_data(req, data, strlen(data), 0);

    free((void *)data);
    cJSON_Delete(obj);

    free(settings);
    return ESP_OK;
}

void buffer_init(CircularBuffer *buffer)
{
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->mutex = xSemaphoreCreateMutex();
    buffer->items = xSemaphoreCreateCounting(DATA_BUFFER_SIZE, 0);
    buffer->spaces = xSemaphoreCreateCounting(DATA_BUFFER_SIZE, DATA_BUFFER_SIZE);
}

esp_err_t extract_zip_to_sd(const char *zip_path, const char *sd_root)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, zip_path, 0))
    {
        ESP_LOGE(TAG, "Failed to open zip file: %s", zip_path);
        return ESP_FAIL;
    }

    int file_count = mz_zip_reader_get_num_files(&zip_archive);
    ESP_LOGI(TAG, "Number of files in zip: %d", file_count);

    char *gcode_filename = malloc(PATH_MAX);  
    if (gcode_filename == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for G-code filename.");
        mz_zip_reader_end(&zip_archive);
        return ESP_FAIL;
    }
    memset(gcode_filename, 0, PATH_MAX);

    char *temp_folder_path = malloc(PATH_MAX);
    if (temp_folder_path == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for temp folder path.");
        free(gcode_filename);
        mz_zip_reader_end(&zip_archive);
        return ESP_FAIL;
    }

    snprintf(temp_folder_path, PATH_MAX, "%s/temp_extract", sd_root);
    mkdir(temp_folder_path, 0777);  

    for (int i = 0; i < file_count; i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            ESP_LOGE(TAG, "Failed to get file stat for file %d", i);
            mz_zip_reader_end(&zip_archive);
            free(gcode_filename);
            free(temp_folder_path);
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "File in zip: %s", file_stat.m_filename);

        char *extracted_file_path = malloc(PATH_MAX);
        if (extracted_file_path == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for extracted file path.");
            mz_zip_reader_end(&zip_archive);
            free(gcode_filename);
            free(temp_folder_path);
            return ESP_FAIL;
        }

        if (strstr(file_stat.m_filename, ".gcode") != NULL)
        {

            snprintf(extracted_file_path, PATH_MAX, "%s/%s", sd_root, file_stat.m_filename);

            if (!mz_zip_reader_extract_to_file(&zip_archive, i, extracted_file_path, 0))
            {
                ESP_LOGE(TAG, "Failed to extract G-code file: %s", file_stat.m_filename);
                mz_zip_reader_end(&zip_archive);
                free(gcode_filename);
                free(temp_folder_path);
                free(extracted_file_path);
                return ESP_FAIL;
            }

            strncpy(gcode_filename, file_stat.m_filename, PATH_MAX - 1);
            char *dot = strrchr(gcode_filename, '.');
            if (dot)
            {
                *dot = '\0';  
            }
        }
        else
        {

            snprintf(extracted_file_path, PATH_MAX, "%s/%s", temp_folder_path, file_stat.m_filename);

            if (!mz_zip_reader_extract_to_file(&zip_archive, i, extracted_file_path, 0))
            {
                ESP_LOGE(TAG, "Failed to extract file: %s", file_stat.m_filename);
                mz_zip_reader_end(&zip_archive);
                free(gcode_filename);
                free(temp_folder_path);
                free(extracted_file_path);
                return ESP_FAIL;
            }
        }

        free(extracted_file_path);  
    }

    if (gcode_filename[0] != '\0')
    {

        char *new_folder_path = malloc(PATH_MAX);
        if (new_folder_path == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for new folder path.");
            mz_zip_reader_end(&zip_archive);
            free(gcode_filename);
            free(temp_folder_path);
            return ESP_FAIL;
        }

        snprintf(new_folder_path, PATH_MAX, "%s/%s", sd_root, gcode_filename);
        if (rename(temp_folder_path, new_folder_path) != 0)
        {
            ESP_LOGE(TAG, "Failed to rename folder: %s to %s", temp_folder_path, new_folder_path);
            mz_zip_reader_end(&zip_archive);
            free(gcode_filename);
            free(temp_folder_path);
            free(new_folder_path);
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Renamed temp folder to: %s", new_folder_path);
        free(new_folder_path);  
    }
    else
    {
        ESP_LOGE(TAG, "No G-code file found. Keeping files in temp folder: %s", temp_folder_path);
    }

    free(gcode_filename);
    free(temp_folder_path);

    mz_zip_reader_end(&zip_archive);
    return ESP_OK;
}

void printingTask(void *params)
{
    while (true)
    {
        if (file_ready_for_extraction)
        {
            ESP_LOGI(TAG, "Extracting ZIP file: %s", zip_file_path);

            if (extract_zip_to_sd(zip_file_path, "/sdcard") == ESP_OK)
            {
                ESP_LOGI(TAG, "ZIP file extracted successfully.");
            }
            else
            {
                ESP_LOGE(TAG, "Failed to extract ZIP file.");
            }

            file_ready_for_extraction = false;
        }
        else if (printed)
        {
            printf("Print Started\n");

            printed = 0;
            printFile(tempFileName);
        }
        vTaskDelay(1);
    }
}

void consumer_task(void *params)
{

    while (true)
    {

        if (xSemaphoreTake(usbbuffer.items, 1))
        {

            if (xSemaphoreTake(usbbuffer.mutex, portMAX_DELAY) == pdTRUE)
            {
                bool send = false;
                char data_to_send[MAX_STRING_LENGTH] = {0};

                if (usbbuffer.count > 0)
                {
                    strcpy(data_to_send, usbbuffer.data[usbbuffer.tail]);
                    usbbuffer.tail = (usbbuffer.tail + 1) % DATA_BUFFER_SIZE;
                    usbbuffer.count--;
                    send = strlen(data_to_send) > 0;
                }

                xSemaphoreGive(usbbuffer.mutex);

                xSemaphoreGive(usbbuffer.spaces);

                if (send)
                {

                    send_message_to_all_ws_clients(data_to_send);
                    memset(data_to_send, 0, sizeof(data_to_send));
                }
            }
        }
        vTaskDelay(1);
    }
}

bool is_file_upload_complete(const char *full_filepath, size_t expected_file_size)
{
    struct stat st;

    if (stat(full_filepath, &st) == 0)
    {

        return st.st_size >= expected_file_size;
    }
    return false;
}

esp_err_t file_upload_handler(httpd_req_t *req)
{
    char *query = malloc(256);
    char *filepath = malloc(256);
    char *file_size_str = malloc(32);
    char *folder = malloc(256);

    size_t expected_file_size = 0;

    if (!query || !filepath || !file_size_str || !folder)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for variables.");
        free(query);
        free(filepath);
        free(file_size_str);
        free(folder);
        return ESP_FAIL;
    }

    if (httpd_req_get_url_query_str(req, query, 256) == ESP_OK)
    {
        ESP_LOGI(TAG, "Found URL query: %s", query);

        if (httpd_query_key_value(query, "filename", filepath, 256) != ESP_OK)
        {
            ESP_LOGE(TAG, "Filename not found in query parameters.");
            const char *resp = "{\"resp\": \"Filename not found.\",\"status\": \"400\"}";
            httpd_resp_send(req, resp, strlen(resp));
            goto cleanup;
        }

        if (httpd_query_key_value(query, "file_size", file_size_str, 32) == ESP_OK)
        {
            expected_file_size = strtoul(file_size_str, NULL, 10);
            ESP_LOGI(TAG, "Expected file size: %zu bytes", expected_file_size);
        }
        else
        {
            ESP_LOGE(TAG, "File size not found in query parameters.");
            const char *resp = "{\"resp\": \"File size not found.\",\"status\": \"400\"}";
            httpd_resp_send(req, resp, strlen(resp));
            goto cleanup;
        }

        if (httpd_query_key_value(query, "file_folder", folder, 256) != ESP_OK)
        {
            ESP_LOGE(TAG, "File Folder not found in query parameters.");
            const char *resp = "{\"resp\": \"File Folder not found.\",\"status\": \"400\"}";
            httpd_resp_send(req, resp, strlen(resp));
            goto cleanup;
        }
    }
    else
    {
        ESP_LOGE(TAG, "No query string found.");
        const char *resp = "{\"resp\": \"No query string found.\",\"status\": \"400\"}";
        httpd_resp_send(req, resp, strlen(resp));
        goto cleanup;
    }

    if (!sdInserted())
    {
        ESP_LOGE(TAG, "SD Card not detected.");
        const char *resp = "{\"resp\": \"SD Card not detected.\",\"status\": \"403\"}";
        httpd_resp_send(req, resp, strlen(resp));
        goto cleanup;
    }

    char *full_filepath = malloc(512 + 8);
    char *full_folderpath = malloc(256 + 8);

    if (!full_filepath || !full_folderpath)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for file paths.");
        goto cleanup;
    }

    if (strncmp(folder, "root", 4) != 0)
    {
        snprintf(full_folderpath, 256 + 8, "/sdcard/%s", folder);

        struct stat st = {0};

        if (stat(full_folderpath, &st) == -1)
        {
            if (mkdir(full_folderpath, 0777) == -1)
            {
                ESP_LOGE(TAG, "Error creating folder: %s", strerror(errno));
            }
            else
            {
                ESP_LOGI(TAG, "Folder created: %s", full_folderpath);
            }
        }
        snprintf(full_filepath, 512 + 8, "/sdcard/%s/%s", folder, filepath);
    }
    else
    {
        snprintf(full_filepath, 512 + 8, "/sdcard/%s", filepath);
    }

    char *buf = (char *)heap_caps_malloc(4096 * 2, MALLOC_CAP_SPIRAM);
    if (buf == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for buffer.");
        goto cleanup_filepaths;
    }

    FILE *file;
    if (is_file_upload_complete(full_filepath, expected_file_size))
    {
        file = fopen(full_filepath, "w");  
    }
    else
    {
        file = fopen(full_filepath, "a");  
    }
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filepath);
        free(buf);
        goto cleanup_filepaths;
    }

    int received = 0;
    int remaining = req->content_len;

    while (remaining > 0)
    {
        int ret = httpd_req_recv(req, buf, MIN(remaining, 4096 * 2));
        if (ret <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive data from request.");
            fclose(file);
            free(buf);
            goto cleanup_filepaths;
        }

        if (fwrite(buf, 1, ret, file) != ret)
        {
            ESP_LOGE(TAG, "Failed to write data to file: %s", filepath);
            fclose(file);
            free(buf);
            goto cleanup_filepaths;
        }
        received += ret;
        remaining -= ret;
    }

    fclose(file);
    free(buf);

    if (is_file_upload_complete(full_filepath, expected_file_size))
    {
        ESP_LOGI(TAG, "File upload complete: %s", filepath);

    }

    const char *resp = "{\"resp\": \"File uploaded successfully\",\"status\": \"200\"}";
    httpd_resp_send(req, resp, strlen(resp));

cleanup_filepaths:
    free(full_filepath);
    free(full_folderpath);

cleanup:
    free(query);
    free(filepath);
    free(file_size_str);
    free(folder);

    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{

    httpd_handle_t server = NULL;

    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
    conf.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &conf) == ESP_OK, "Start server failed", err_start);
    init_ws_server();
    httpd_register_uri_handler(server, &ws);

    httpd_uri_t file_upload_post_uri = {
        .uri = "/api/v1/file_upload",
        .method = HTTP_POST,
        .handler = file_upload_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &file_upload_post_uri);

    httpd_uri_t options_uri = {
        .uri = "/*",
        .method = HTTP_OPTIONS,
        .handler = options_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &options_uri);

    httpd_uri_t sd_data_get_uri = {
        .uri = "/api/v1/sdlist",
        .method = HTTP_GET,
        .handler = sd_data_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &sd_data_get_uri);

    httpd_uri_t sd_data_get_thumb_uri = {
        .uri = "/api/v1/getthumb",
        .method = HTTP_GET,
        .handler = gcode_thumbnail_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &sd_data_get_thumb_uri);

    httpd_uri_t post_uri = {
        .uri = "/api/v1/printer",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &post_uri);

    httpd_uri_t getdata_uri = {
        .uri = "/api/v1/getdata",
        .method = HTTP_GET,
        .handler = rest_getdata_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &getdata_uri);

    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context};
    httpd_register_uri_handler(server, &common_get_uri);

    buffer_init(&usbbuffer);

    global_printer_settings = getPrinterSettings();

    char *endptr;

    if (global_printer_settings == NULL)
    {
        printf("setting is null");
    }

    if (global_printer_settings != NULL)
    {
        *offX = strtod(global_printer_settings->coord_x_adj, &endptr);         
        *offY = strtod(global_printer_settings->coord_y_adj, &endptr);         
        printHeadAlign = strtod(global_printer_settings->align_adj, &endptr);  
    }

    printf("xoff:%f xoff:%f align:%d\n", *offX, *offY, printHeadAlign);

    xTaskCreatePinnedToCore(consumer_task, "ConsumerTask", 2048 * 8, NULL, 6, NULL, 1);
    xTaskCreatePinnedToCore(printingTask, "printingTask", 2048 * 8, NULL, 6, NULL, 1);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
