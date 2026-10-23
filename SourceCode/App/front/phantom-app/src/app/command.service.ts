/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Injectable } from '@angular/core';
import { BehaviorSubject,Observable } from 'rxjs';
import { ApiService } from './api.service';
import { PrinterCommand } from './printer';
@Injectable({
    providedIn: 'root'
})
export class CommandService {
    private selectedFileSubject = new BehaviorSubject<any | null>(null);
    constructor(private api: ApiService) {
    }
    setSelectedFile(file: any): void {
        this.selectedFileSubject.next(file);
    }
    getSelectedFile(): Observable<any | null> {
        return this.selectedFileSubject.asObservable();
    }
    sendCommand(command: PrinterCommand) {
        this.api.postCommand(command).subscribe((_x) => { });
    }
    getCommand(command: PrinterCommand): Observable<any> {
        return this.api.get(command, 'getdata');
    }
}
