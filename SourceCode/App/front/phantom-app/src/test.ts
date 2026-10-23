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
import { getTestBed } from '@angular/core/testing';
import { BrowserDynamicTestingModule,platformBrowserDynamicTesting } from '@angular/platform-browser-dynamic/testing';
import 'zone.js/dist/zone-testing';
declare const require: {
    context(path: string, deep?: boolean, filter?: RegExp): {
        keys(): string[];
        <T>(id: string): T;
    };
};
getTestBed().initTestEnvironment(BrowserDynamicTestingModule, platformBrowserDynamicTesting());
const context = require.context('./', true, /\.spec\.ts$/);
context.keys().map(context);
