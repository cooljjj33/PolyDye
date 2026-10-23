/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Directive,ElementRef,HostListener } from '@angular/core';
@Directive({
    selector: '[autoGrow]'
})
export class AutoGrowDirective {
    constructor(private el: ElementRef) {
    }
    @HostListener('input', ['$event.target'])
    onInput(_textArea: HTMLTextAreaElement): void {
        this.adjust();
    }
    ngAfterViewInit(): void {
        setTimeout(() => this.adjust(), 0);
    }
    adjust(): void {
        const textArea = this.el.nativeElement;
        textArea.style.overflow = 'hidden';
        textArea.style.height = 'auto';
        textArea.style.height = textArea.scrollHeight + 'px';
    }
}
