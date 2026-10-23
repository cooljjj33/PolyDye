/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { HostListener,Injectable } from '@angular/core';
import { Subject } from 'rxjs';
@Injectable({
    providedIn: 'root'
})
export class DatasockService {
    private dataSource = new Subject<any>();
    private websocket!: WebSocket;
    constructor() { }
    recon: String = "";
    data$ = this.dataSource.asObservable();
    updateData(data: any) {
        this.dataSource.next(data);
    }
    sendCommand(message) {
        if (this.websocket) {
            if (this.websocket.readyState === WebSocket.OPEN) {
                if (message.trim() != "") {
                    message = "\r" + message + "\r";
                    this.recon = message;
                    this.websocket.send(message);
                    message = "";
                }
            }
        }
        return message + '\n';
    }
    connectWebsocket(): void {
        if (!this.websocket) {
            const host = window.location.hostname;
            const port = window.location.port ? `:${window.location.port}` : '';
            const path = '/ws';
            this.websocket = new WebSocket(`${"ws://"}${host}${port}${path}`);
            this.websocket.onopen = (_event) => {
                if (this.recon.trim() != "") {
                    this.recon = "";
                }
            };
            this.websocket.onmessage = (event) => {
                this.updateData(event.data);
            };
            this.websocket.onerror = (event) => {
                console.error('WebSocket Error', event);
                this.websocket = null;
                console.log('WebSocket is closed now.');
                if (currentReconnectAttempts < ++maxReconnectAttempts) {
                    let reconnectDelay = initialReconnectDelay;
                    setTimeout(() => {
                        console.log('Attempting to reconnect...');
                        this.connectWebsocket();
                    }, reconnectDelay);
                }
                else {
                    console.log('Max reconnect attempts reached. Not trying to reconnect.');
                    this.recon = "";
                }
            };
            let maxReconnectAttempts = 5;
            let currentReconnectAttempts = 0;
            let initialReconnectDelay = 1000;
            this.websocket.onclose = (_event) => {
                this.websocket = null;
                console.log('WebSocket is closed now.');
                if (currentReconnectAttempts < ++maxReconnectAttempts) {
                    let reconnectDelay = initialReconnectDelay;
                    setTimeout(() => {
                        console.log('Attempting to reconnect...');
                        this.connectWebsocket();
                    }, reconnectDelay);
                }
                else {
                    console.log('Max reconnect attempts reached. Not trying to reconnect.');
                    this.recon = "";
                }
            };
        }
    }
    @HostListener('window:beforeunload', ['$event'])
    unloadHandler(_event: Event): void {
        if (this.websocket) {
            this.websocket.close();
        }
    }
}
