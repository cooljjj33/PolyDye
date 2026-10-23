/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component,OnInit } from '@angular/core';
import { faArrowDown,faArrowLeft,faArrowRight,faArrowUp,faHome } from '@fortawesome/free-solid-svg-icons';
import { CommandService } from '../command.service';
import { PrinterCommand,PrinterCommandType } from '../printer';
@Component({
    selector: 'app-printercontrol',
    templateUrl: './printercontrol.component.html',
    styleUrls: ['./printercontrol.component.scss']
})
export class PrintercontrolComponent implements OnInit {
    faArrowLeft = faArrowLeft;
    faArrowRight = faArrowRight;
    faArrowUp = faArrowUp;
    faArrowDown = faArrowDown;
    faHome = faHome;
    payload: PrinterCommand = null;
    selectedMoveAmount: number = 0.2;
    selectedAlignAmount: number = 2000;
    constructor( private commandService: CommandService) {
        this.payload = {
            id: -1,
            command: PrinterCommandType.None,
            parameters: {
                temperature_Hotend: 100
            }
        };
    }
    ngOnInit(): void {
    }
    setMoveAmount(amount: number) {
        this.selectedMoveAmount = amount;
    }
    setAlignAmount(amount: number) {
        this.selectedAlignAmount = amount;
    }
    numSign(isPositive, num) {
        if (isPositive) {
            if (num < 0) {
                num *= -1;
            }
        }
        else {
            if (num > 0) {
                num *= -1;
            }
        }
        return num;
    }
    sendMove(dir: string) {
        let command: PrinterCommand;
        switch (dir) {
            case 'Left':
                command = {
                    id: PrinterCommandType.Move,
                    command: PrinterCommandType.Move,
                    parameters: {
                        x: this.numSign(false, this.selectedMoveAmount)
                    }
                };
                break;
            case 'Right':
                command = {
                    id: PrinterCommandType.Move,
                    command: PrinterCommandType.Move,
                    parameters: {
                        x: this.numSign(true, this.selectedMoveAmount)
                    }
                };
                break;
            case 'Back':
                command = {
                    id: PrinterCommandType.Move,
                    command: PrinterCommandType.Move,
                    parameters: {
                        y: this.numSign(true, this.selectedMoveAmount)
                    }
                };
                break;
            case 'Front':
                command = {
                    id: PrinterCommandType.Move,
                    command: PrinterCommandType.Move,
                    parameters: {
                        y: this.numSign(false, this.selectedMoveAmount)
                    }
                };
                break;
            default:
                { }
                break;
        }
        this.commandService.sendCommand(command);
    }
    sendAlign(dir: string) {
        let command: PrinterCommand;
        switch (dir) {
            case 'Up':
                command = {
                    id: PrinterCommandType.Align,
                    command: PrinterCommandType.Align,
                    parameters: {
                        u: this.numSign(false, this.selectedAlignAmount)
                    }
                };
                break;
            case 'Down':
                command = {
                    id: PrinterCommandType.Align,
                    command: PrinterCommandType.Align,
                    parameters: {
                        d: this.numSign(true, this.selectedAlignAmount)
                    }
                };
                break;
            default:
                { }
                break;
        }
        this.commandService.sendCommand(command);
    }
}
