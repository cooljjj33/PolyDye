/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component,ElementRef,OnInit,QueryList,ViewChild,ViewChildren } from '@angular/core';
import { Subscription } from 'rxjs';
import { DatasockService } from '../datasock.service';
@Component({
    selector: 'app-terminal',
    templateUrl: './terminal.component.html',
    styleUrls: ['./terminal.component.scss']
})
export class TerminalComponent implements OnInit {
    @ViewChild('scrollframe', { static: false })
    scrollFrame: ElementRef;
    @ViewChildren('item')
    itemElements: QueryList<any>;
    scrollContainer: any;
    items = [];
    isNearBottom = true;
    message: string;
    debounceTimer: any;
    debounceInterval: number;
    messageBuffer: string[];
    myData: string = "";
    subscription: Subscription;
    constructor(private dataService: DatasockService) {
        this.messageBuffer = [];
        this.debounceTimer = null;
        this.debounceInterval = 100;
        this.myData = "";
        this.items = [];
        this.subscription = this.dataService.data$.subscribe(data => {
            this.bufferMessage(data);
        });
    }
    ngOnInit(): void {
        this.items = new Array();
        this.dataService.connectWebsocket();
    }
    ngOnDestroy() {
        this.subscription.unsubscribe();
    }
    sendCommand() {
        this.message = this.dataService.sendCommand(this.message + '\n');
    }
    ngAfterViewInit() {
        this.scrollContainer = this.scrollFrame.nativeElement;
        this.itemElements.changes.subscribe(_ => this.onItemElementsChanged());
    }
    private onItemElementsChanged(): void {
        if (this.isNearBottom) {
            this.scrollToBottom();
        }
    }
    private scrollToBottom(): void {
        this.scrollContainer.scroll({
            top: this.scrollContainer.scrollHeight,
            left: 0,
            behavior: 'smooth'
        });
    }
    private isUserNearBottom(): boolean {
        const threshold = 150;
        const position = this.scrollContainer.scrollTop + this.scrollContainer.offsetHeight;
        const height = this.scrollContainer.scrollHeight;
        return position > height - threshold;
    }
    scrolled(_event: any): void {
        this.isNearBottom = this.isUserNearBottom();
    }
    bufferMessage(data) {
        this.messageBuffer.push(data);
        if (this.debounceTimer) {
            clearTimeout(this.debounceTimer);
        }
        this.debounceTimer = setTimeout(() => {
            this.processMessages();
        }, this.debounceInterval);
    }
    processMessages() {
        let combinedData = this.messageBuffer.join('').replace(/[^\x20-\x7E\n\r]/g, '');
        if (combinedData.indexOf("\n") < 0) {
            this.myData += combinedData;
        }
        else {
            this.myData = combinedData;
        }
        if (this.myData.indexOf("\n") > -1) {
            this.items.push(...this.myData.split("\n").filter(item => item.trim() !== ""));
            this.myData = "";
        }
        while (this.items.length > 300) {
            this.items.shift();
        }
        this.messageBuffer = [];
    }
    onKeyDown(event: KeyboardEvent) {
        if (event.key === 'Enter') {
            if (this.message.length > 0) {
                this.sendCommand();
            }
        }
    }
}
