/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
#pragma once

typedef struct {
    int bedsize_x;
    int bedsize_y;
    int bedsize_z;
    char cam_url[256];
    int extruder_fr;
    int travel_fr;
    int travel_accel;
    int travel_accel_z;
    int print_accel;
    char coord_x_adj[32];
    char coord_y_adj[32];
    char align_adj[32];

} printer_settings_t;
extern printer_settings_t *global_printer_settings;
extern int printHeadAlign;
extern int printstart;


