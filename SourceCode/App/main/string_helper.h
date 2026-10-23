/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
#pragma once
#include <ctype.h>

static char *caps(char str[2048])
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] >= 'a' && str[i] <= 'z')
        {
            str[i] = str[i] - 32;
        }
    }
    return str;
}

char *ltrim(char *s)
{
    while (isspace(*s))
        s++;
    return s;
}

static bool isComment(char str[2048])
{
    bool result = false;

    char *s = ltrim(str);
    result = s[0] == ';' || strlen(s) == 0 || s[0] ==' ' || s[0] == '\n' || s[0] == '\r';
    return result;
}

static bool isOk(char str[2048])
{
    int count = 0;
    char *s = ltrim(str);

    if (strlen(s) == 0) {
        return 1;
    }

    const char *keywords[] = { "OK", "ok", "Ok", "oK"};
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

    for (int i = 0; i < num_keywords; i++) {
        char *ptr = s;
        while ((ptr = strstr(ptr, keywords[i])) != NULL) {
            count++;
            ptr += strlen(keywords[i]);
        }
    }

    return count > 0;
}
