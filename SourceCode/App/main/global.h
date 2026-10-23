/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
#pragma once

extern char *myBuf[1000];
extern int myBufPos;
#define DATA_BUFFER_SIZE 10
#define MAX_STRING_LENGTH 2048

extern int inkSplurt;
extern int printed;

typedef struct {
    char data[DATA_BUFFER_SIZE][MAX_STRING_LENGTH];
    int head;
    int tail;
    int count;
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t items;
    SemaphoreHandle_t spaces;
} CircularBuffer;

extern CircularBuffer usbbuffer;

bool sdInserted()
{
    return !gpio_get_level((gpio_num_t)39);
}
