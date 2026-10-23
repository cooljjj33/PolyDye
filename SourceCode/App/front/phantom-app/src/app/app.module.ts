/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-FileCopyrightText: 2017 Google, Inc.
 * SPDX-License-Identifier: LicenseRef-NCL-1.0 AND MIT
 * 
 * CMY Labs PTY LTD's original copyrightable additions are NCL-1.0 ONLY.
 * Retained Angular CLI-generated scaffold expression remains under MIT.
 * The licenses apply to their respective contributions, not as alternatives.
 * Modified in this project; upstream notice: LICENSES/Angular-CLI-MIT.txt.
 * Paths are relative to the repository root.
 * Terms: LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { HttpClientModule } from '@angular/common/http';
import { NgModule } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { BrowserModule } from '@angular/platform-browser';
import { FontAwesomeModule } from '@fortawesome/angular-fontawesome';
import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { ConnectionComponent } from './connection/connection.component';
import { DashboardComponent } from './dashboard/dashboard.component';
import { AutoGrowDirective } from './directives/auto-grow.directive';
import { FilemanagerComponent } from './filemanager/filemanager.component';
import { ImageDisplayComponent } from './image-display/image-display.component';
import { PrinterSettingsModalComponent } from './printer-settings-modal/printer-settings-modal.component';
import { PrintercontrolComponent } from './printercontrol/printercontrol.component';
import { PrintscreenComponent } from './printscreen/printscreen.component';
import { SettingsModalService } from './settings-modal.service';
import { TerminalComponent } from './terminal/terminal.component';
import { WifiSettingsModalComponent } from './wifi-settings-modal/wifi-settings-modal.component';
@NgModule({
    declarations: [
        AppComponent,
        DashboardComponent,
        FilemanagerComponent,
        PrintercontrolComponent,
        TerminalComponent,
        ConnectionComponent,
        WifiSettingsModalComponent,
        PrinterSettingsModalComponent,
        AutoGrowDirective,
        ImageDisplayComponent,
        PrintscreenComponent
    ],
    imports: [
        BrowserModule,
        FormsModule,
        AppRoutingModule,
        HttpClientModule, FontAwesomeModule
    ],
    providers: [SettingsModalService],
    bootstrap: [AppComponent]
})
export class AppModule {
}
