/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
export enum PrinterCommandType {
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
}
export interface PrinterCommand {
    id: number;
    command: PrinterCommandType;
    parameters?: {
        [key: string]: any;
    };
}
export interface PrinterDirCommand {
    command: PrinterCommandType;
    fileList: {
        [key: string]: any;
    };
    fileCount: number;
    status: string;
}
export interface PrinterSettings {
    bedsize_x: number;
    bedsize_y: number;
    bedsize_z: number;
    cam_url: string;
    extruder_fr: number;
    travel_fr: number;
    travel_accel: number;
    travel_accel_z: number;
    print_accel: number;
    coord_x_adj: string;
    coord_y_adj: string;
    align_adj: string;
}
