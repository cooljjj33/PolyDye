/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 *
 * CMY Labs PTY LTD's original content; NCL-1.0 ONLY.
 * See the repository-root LICENSE and licensing/MANIFEST.json.
 */

#pragma once
#include <stdint.h>
#include <esp_https_server.h>

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

static httpd_handle_t server = NULL;
