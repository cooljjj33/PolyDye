/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component,OnDestroy,OnInit } from '@angular/core';
import { faArrowDown,faArrowRight } from '@fortawesome/free-solid-svg-icons';
import { Subscription } from 'rxjs';
import { CommandService } from '../command.service';
@Component({
    selector: 'app-connection',
    templateUrl: './connection.component.html',
    styleUrls: ['./connection.component.scss']
})
export class ConnectionComponent implements OnInit, OnDestroy {
    selectedFile: string | null = "No File Selected";
    private fileSubscription: Subscription;
    faArrowRight = faArrowRight;
    faArrowDown = faArrowDown;
    settingsCollapsed = false;
    infoCollapsed = false;
    thumbnailSrc: string | null = null;
    constructor(private commandService: CommandService) { }
    ngOnInit(): void {
        this.fileSubscription = this.commandService.getSelectedFile().subscribe(file => {
            if (file) {
                this.selectedFile = file.selectedFilename;
                this.thumbnailSrc = file.thumbnail;
            }
        });
    }
    ngOnDestroy(): void {
        if (this.fileSubscription) {
            this.fileSubscription.unsubscribe();
        }
    }
}
