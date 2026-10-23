/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component,OnInit } from '@angular/core';
import { faArrowLeft,faArrowRight,faArrowUp,faBars,faHome } from '@fortawesome/free-solid-svg-icons';
import { Subscription } from 'rxjs';
import { PrinterSettings } from '../printer';
import { SettingsModalService } from '../settings-modal.service';
@Component({
    selector: 'app-dashboard',
    templateUrl: './dashboard.component.html',
    styleUrls: ['./dashboard.component.scss']
})
export class DashboardComponent implements OnInit {
    faArrowLeft = faArrowLeft;
    faArrowRight = faArrowRight;
    faArrowUp = faArrowUp;
    faBars = faBars;
    faHome = faHome;
    isOffset = false;
    files: any[];
    version = "1.17";
    public printerSettingsSubscription: Subscription;
    public printerSettings: PrinterSettings;
    constructor( private settingsModalService: SettingsModalService) {
        this.printerSettingsSubscription = this.settingsModalService.printerSettings$.subscribe(settings => {
            this.printerSettings = settings;
        });
    }
    ngOnInit(): void {
    }
    openWifiModal() {
        this.settingsModalService.openWifiModal();
    }
    openPrinterModal() {
        this.settingsModalService.openPrinterModal();
    }
    openOffsets() {
        this.settingsModalService.setOffsetOpen(true);
        this.isOffset = true;
    }
    getOffset() {
        return this.settingsModalService.getOffsetOpen();
    }
}
