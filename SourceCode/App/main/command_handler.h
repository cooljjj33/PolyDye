/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
#define STATUS_OK "Received"
#define STATUS_DIR_GONE "Directory does not exist"
#include "cJSON.h"

typedef struct {
    char fileName[256];
    char base64Img[5000];
} sd_file_t;

typedef struct {
    int command;
    sd_file_t *fileList[40];
    int fileCount;
    char * status;
} printer_dir_command_t;

typedef struct {
    int id;
    int command;
    cJSON *parameters;
} printer_command_t;

enum PrinterCommandType
{
  None,
  Move,
  Extrude,
  Retract,
  SetBed,
  SetHotend,
  RefreshSD,
  SelectFile,
  PrintFile,
  Login,
  Settings,
  Align
};
