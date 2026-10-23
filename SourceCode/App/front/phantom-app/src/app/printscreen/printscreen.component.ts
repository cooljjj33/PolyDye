/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component } from '@angular/core';
import { Subscription } from 'rxjs';
import { DatasockService } from '../datasock.service';
@Component({
    selector: 'app-printscreen',
    templateUrl: './printscreen.component.html',
    styleUrls: ['./printscreen.component.scss']
})
export class PrintscreenComponent {
    progress: number = 0;
    hours: number = 0;
    minutes: number = 0;
    subscription: Subscription;
    bedTemp: string;
    nozzleTemp: string;
    constructor(private dataService: DatasockService) {
        this.subscription = this.dataService.data$.subscribe(data => {
            this.processMessage(data);
        });
    }
    cancelPrint() {
        this.dataService.sendCommand("Print Aborted\n");
        this.dataService.sendCommand("M106 S0\n");
        this.dataService.sendCommand("M104 S0\n");
        this.dataService.sendCommand("M140 S0\n");
        this.dataService.sendCommand("M84 X Y E\n");
    }
    extractValues(message: string): [
        string,
        string
    ] | null {
        const parts = message.split(' ');
        let tValue = '';
        let bValue = '';
        for (const part of parts) {
            if (part.startsWith('T:')) {
                tValue = part.split('/')[0].replace('T:', '');
            }
            else if (part.startsWith('B:')) {
                bValue = part.split('/')[0].replace('B:', '');
            }
        }
        if (tValue && bValue) {
            return [tValue, bValue];
        }
        return null;
    }
    processMessage(data) {
        const temps = this.extractValues(data);
        if (temps) {
            this.nozzleTemp = temps[0];
            this.bedTemp = temps[1];
        }
        const regex = /M73 P(\d+) R(\d+)/;
        const match = data.match(regex);
        if (match) {
            const percentage = parseInt(match[1], 10);
            const remainingMinutes = parseInt(match[2], 10);
            this.hours = Math.floor(remainingMinutes / 60);
            this.minutes = remainingMinutes % 60;
            this.progress = percentage;
        }
    }
}
