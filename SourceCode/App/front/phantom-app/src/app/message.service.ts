/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Injectable } from '@angular/core';
@Injectable({ providedIn: 'root' })
export class MessageService {
    messages: string[] = [];
    add(message: string) {
        this.messages.push(message);
    }
}
