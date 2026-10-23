/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs';
import { PrinterSettings } from './printer';
@Injectable({
    providedIn: 'root'
})
export class SettingsModalService {
    private isWifiOpenSubject = new BehaviorSubject<boolean>(false);
    public isWifiOpen$ = this.isWifiOpenSubject.asObservable();
    private isPrinterOpenSubject = new BehaviorSubject<boolean>(false);
    public isPrinterOpen$ = this.isPrinterOpenSubject.asObservable();
    public isOffsetOpen = false;
    private printerSettingsSubject = new BehaviorSubject<PrinterSettings>({
        bedsize_x: 235,
        bedsize_y: 235,
        bedsize_z: 250,
        cam_url: "",
        extruder_fr: 10,
        travel_fr: 100,
        travel_accel: 500,
        travel_accel_z: 10,
        print_accel: 500,
        coord_x_adj: "0.00",
        coord_y_adj: "0.00",
        align_adj: "70000",
    });
    public printerSettings$ = this.printerSettingsSubject.asObservable();
    constructor() {
    }
    public openWifiModal() {
        this.isWifiOpenSubject.next(true);
    }
    public openPrinterModal() {
        this.isPrinterOpenSubject.next(true);
    }
    public setOffsetOpen(value) {
        this.isOffsetOpen = value;
    }
    public getOffsetOpen() {
        return this.isOffsetOpen;
    }
}
