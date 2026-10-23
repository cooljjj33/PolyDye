/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component,OnInit } from '@angular/core';
import { CommandService } from '../command.service';
import { PrinterCommand,PrinterCommandType } from '../printer';
import { SettingsModalService } from '../settings-modal.service';
@Component({
    selector: 'app-wifi-settings-modal',
    templateUrl: './wifi-settings-modal.component.html',
    styleUrls: ['./wifi-settings-modal.component.scss']
})
export class WifiSettingsModalComponent implements OnInit {
    public isModalOpen: boolean = false;
    public ssid: string;
    public password: string;
    constructor(private settingsModalService: SettingsModalService, private commandService: CommandService) {
        this.settingsModalService.isWifiOpen$.subscribe(isOpen => {
            this.isModalOpen = isOpen;
            if (this.isModalOpen) {
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
            id: PrinterCommandType.Login,
            command: PrinterCommandType.Login,
            parameters: {
                ssid: this.ssid,
                password: this.password
            }
        };
        this.commandService.sendCommand(command);
        this.closeModal();
    }
    closeModal() {
        this.isModalOpen = false;
    }
    openModal() {
        this.isModalOpen == true;
    }
}
