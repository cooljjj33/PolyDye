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
import { ComponentFixture,TestBed } from '@angular/core/testing';
import { PrintscreenComponent } from './printscreen.component';
describe('PrintscreenComponent', () => {
    let component: PrintscreenComponent;
    let fixture: ComponentFixture<PrintscreenComponent>;
    beforeEach(() => {
        TestBed.configureTestingModule({
            declarations: [PrintscreenComponent]
        });
        fixture = TestBed.createComponent(PrintscreenComponent);
        component = fixture.componentInstance;
        fixture.detectChanges();
    });
    it('should create', () => {
        expect(component).toBeTruthy();
    });
});
