/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { HttpClient,HttpHeaders } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Observable,of } from 'rxjs';
import { catchError,tap } from 'rxjs/operators';
import { environment } from 'src/environments/environment';
import { MessageService } from './message.service';
import { PrinterCommand } from './printer';
@Injectable({
    providedIn: 'root'
})
export class ApiService {
    private apiUrl = environment.apiUrl;
    httpOptions = {
        headers: new HttpHeaders({
            'Content-Type': 'application/json',
            'Access-Control-Allow-Headers': 'Content-Type, Access-Control-Allow-Headers, Authorization, X-Requested-With',
            'Access-Control-Allow-Origin': '*'
        })
    };
    constructor(private http: HttpClient, private messageService: MessageService) { }
    get(_command: PrinterCommand, uri: string): Observable<any> {
        const baseUrl = window.location.hostname;
        const apiUrl = `/api/v1/${uri}`;
        console.log(baseUrl);
        console.log(apiUrl);
        console.log(this.apiUrl);
        return this.http.get<any>(apiUrl, this.httpOptions)
            .pipe(tap(_ => this.log(`Fetched data from ${apiUrl}`)), catchError(this.handleError<PrinterCommand>(`GET error for ${uri}`)));
    }
    postCommand(command: PrinterCommand): Observable<PrinterCommand> {
        const apiUrl = `/api/v1/printer`;
        const payload = JSON.stringify(command);
        return this.http.post<PrinterCommand>(apiUrl, payload, this.httpOptions)
            .pipe(tap(() => console.log(`Command sent successfully to ${apiUrl}`)), catchError(this.handleError<PrinterCommand>('sendCommand')));
    }
    private handleError<T>(operation = 'operation', result?: T) {
        return (error: any): Observable<T> => {
            console.error(error);
            this.log(`${operation} failed: ${error.message}`);
            return of(result as T);
        };
    }
    private log(message: string) {
        this.messageService.add(`apiService: ${message}`);
    }
}
