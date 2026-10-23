/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component,OnInit } from '@angular/core';
import { BehaviorSubject,Subscription } from 'rxjs';
import { CommandService } from '../command.service';
import { PrinterCommand,PrinterCommandType,PrinterSettings } from '../printer';
import { SettingsModalService } from '../settings-modal.service';
@Component({
    selector: 'app-printer-settings-modal',
    templateUrl: './printer-settings-modal.component.html',
    styleUrls: ['./printer-settings-modal.component.scss']
})
export class PrinterSettingsModalComponent implements OnInit {
    public isModalOpen: boolean = false;
    public printerSettingsSubscription: Subscription;
    public printerSettings: PrinterSettings;
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
        coord_y_adj: "    0.00",
        align_adj: "70000",
    });
    public printerSettings$ = this.printerSettingsSubject.asObservable();
    constructor(private printerSettingsModalService: SettingsModalService, private commandService: CommandService) {
        this.printerSettingsSubscription = this.printerSettingsModalService.printerSettings$.subscribe(settings => {
            this.printerSettings = settings;
        });
        this.printerSettingsModalService.isPrinterOpen$.subscribe(isOpen => {
            this.isModalOpen = isOpen;
            if (this.isModalOpen) {
                const command: PrinterCommand = {
                    id: PrinterCommandType.Settings,
                    command: PrinterCommandType.Settings,
                    parameters: {}
                };
                this.commandService.getCommand(command).subscribe(x => {
                    this.printerSettings = x.parameters as PrinterSettings;
                    this.printerSettingsSubject.next(this.printerSettings);
                });
                this.openModal();
            }
            else {
                this.closeModal();
            }
        });
    }
    ngOnInit(): void {
    }
    onSubmit() {
        const command: PrinterCommand = {
            id: PrinterCommandType.Settings,
            command: PrinterCommandType.Settings,
            parameters: this.printerSettings
        };
        if (this.printerSettings.align_adj.indexOf("Busy") <= 0 || this.printerSettings.coord_x_adj.indexOf("Busy") <= 0 || this.printerSettings.coord_y_adj.indexOf("Busy") <= 0) {
            this.commandService.sendCommand(command);
        }
        else {
            console.log("PRINTER BUSY");
        }
        this.closeModal();
    }
    closeModal() {
        this.isModalOpen = false;
    }
    openModal() {
        this.isModalOpen == true;
    }
}
