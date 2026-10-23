/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { Component,ElementRef,OnInit,ViewChild } from '@angular/core';
import { SafeUrl } from '@angular/platform-browser';
import { faArrowDown,faArrowLeft,faArrowRight,faArrowUp,faHome } from '@fortawesome/free-solid-svg-icons';
import * as base64js from 'base64-js';
import { saveAs } from 'file-saver';
import * as JSZip from 'jszip';
import pica from 'pica';
import { environment } from 'src/environments/environment';
import { ApiService } from '../api.service';
import { CommandService } from '../command.service';
import { PrinterCommand,PrinterCommandType,PrinterDirCommand } from '../printer';
@Component({
    selector: 'app-filemanager',
    templateUrl: './filemanager.component.html',
    styleUrls: ['./filemanager.component.scss']
})
export class FilemanagerComponent implements OnInit {
    @ViewChild('fileInputUpload')
    fileInput: ElementRef;
    @ViewChild('imageElement')
    imageElement: ElementRef<HTMLImageElement>;
    @ViewChild('myCanvas')
    myCanvas: ElementRef<HTMLCanvasElement>;
    faArrowLeft = faArrowLeft;
    faArrowRight = faArrowRight;
    faArrowUp = faArrowUp;
    faArrowDown = faArrowDown;
    faHome = faHome;
    payload: PrinterDirCommand = null;
    selectedFile: string = "";
    imageSrc: SafeUrl;
    progress: number = 0;
    busyUploading: boolean = false;
    imageCount: number = 0;
    private zip = new JSZip();
    gcodeContent = '';
    gcodeFileName = '';
    constructor( private api: ApiService, private commandService: CommandService) {
        this.payload = {
            command: PrinterCommandType.None,
            status: "Received",
            fileList: [{ fileName: "N/A" }],
            fileCount: 0
        };
    }
    ngOnInit(): void {
    }
    refresh() {
        this.api.get(null, 'sdlist').subscribe((x) => {
            this.payload = x;
        });
    }
    upload() {
        this.fileInput.nativeElement.click();
    }
    selectFile(file) {
        this.selectedFile = file.fileName;
        this.api.get(null, 'getthumb?filename=' + this.selectedFile).subscribe((x) => {
            const lines = x.resp.split('\n');
            let base64Data = '';
            for (const line of lines) {
                base64Data += line.replace('; ', '').trim();
            }
            let thumb = "";
            if (base64Data) {
                thumb = `data:image/png;base64,${base64Data}`;
            }
            else {
            }
            const selectedFileData = {
                "selectedFilename": this.selectedFile,
                "thumbnail": thumb
            };
            this.commandService.setSelectedFile(selectedFileData);
        });
    }
    printFile() {
        const command: PrinterCommand = {
            id: PrinterCommandType.PrintFile,
            command: PrinterCommandType.PrintFile,
            parameters: {
                fileName: this.selectedFile.substring(0, this.selectedFile.lastIndexOf("."))
            }
        };
        this.commandService.sendCommand(command);
    }
    async onFilesSelected(event) {
        const files = event.target.files;
        if (files.length > 0) {
            for (let file of files) {
                try {
                    this.sendFileToESP(file, file.name);
                    break;
                }
                catch (error) {
                    break;
                }
            }
        }
    }
    private sendFileToESP(file: File, filename: string, folder: string = "root"): Promise<any> {
        return new Promise((resolve, reject) => {
            if (folder == "") {
                folder = "root";
            }
            const endpoint = `${environment.apiUrl}api/v1/file_upload?filename=${encodeURIComponent(filename)}&file_size=${file.size}&file_folder=${folder}`;
            const chunkSize = 1024 * 1024;
            const totalChunks = Math.ceil(file.size / chunkSize);
            let currentChunk = 0;
            const uploadChunk = () => {
                const start = currentChunk * chunkSize;
                const end = Math.min(file.size, start + chunkSize);
                const blob = file.slice(start, end);
                const reader = new FileReader();
                reader.onload = () => {
                    const xhr = new XMLHttpRequest();
                    xhr.open("POST", endpoint, true);
                    xhr.setRequestHeader("Content-Type", "application/octet-stream");
                    xhr.upload.onprogress = (event) => {
                        if (event.lengthComputable) {
                            this.progress = Math.round(100 * (event.loaded / (event.total * totalChunks)) + (currentChunk * 100 / totalChunks));
                        }
                    };
                    xhr.onload = () => {
                        if (xhr.status === 200) {
                            currentChunk++;
                            if (currentChunk < totalChunks) {
                                uploadChunk();
                            }
                            else {
                                this.busyUploading = false;
                                resolve(xhr.responseText);
                            }
                        }
                        else {
                            console.error(`Error sending file ${filename}: ${xhr.statusText}`);
                            this.busyUploading = false;
                            reject(new Error(`Error sending file ${filename}: ${xhr.statusText}`));
                        }
                    };
                    xhr.onerror = (error) => {
                        console.error(`Error sending file ${filename}: ${error}`);
                        this.busyUploading = false;
                        reject(error);
                    };
                    xhr.send(reader.result);
                };
                reader.onerror = (error) => {
                    console.error(`Error reading file ${filename}: ${error}`);
                    this.busyUploading = false;
                    reject(error);
                };
                reader.readAsArrayBuffer(blob);
            };
            this.busyUploading = true;
            uploadChunk();
        });
    }
    private async convertImageToThumbnailBlock(imagePath: File): Promise<string> {
        try {
            const reader = new FileReader();
            const imageSrc = await new Promise<string>((resolve, reject) => {
                reader.onload = () => resolve(reader.result as string);
                reader.onerror = reject;
                reader.readAsDataURL(imagePath);
            });
            const img = new Image();
            await new Promise<any>((resolve, reject) => {
                img.onload = resolve;
                img.onerror = reject;
                img.src = imageSrc;
            });
            const canvas = document.createElement('canvas');
            canvas.width = 300;
            canvas.height = 300;
            const picaInstance = pica();
            await picaInstance.resize(img, canvas);
            const imageBuffer = await new Promise<Blob>((resolve) => canvas.toBlob(resolve));
            const arrayBuffer = await imageBuffer.arrayBuffer();
            const base64String = base64js.fromByteArray(new Uint8Array(arrayBuffer));
            const base64Lines = base64String.match(/.{1,80}/g)?.join('\n; ') || '';
            const thumbnailBlock = `; THUMBNAIL_BLOCK_START

    ;
    ; thumbnail begin 300x300 ${arrayBuffer.byteLength}
    ; ${base64Lines}
    ; thumbnail end
    ; THUMBNAIL_BLOCK_END`;
            return thumbnailBlock;
        }
        catch (error) {
            console.error('Error processing image:', error);
        }
    }
    private async replaceThumbnailBlock(fileContent: string, newThumbnailBlock: string): Promise<string> {
        const startTag = '; THUMBNAIL_BLOCK_START';
        const endTag = '; THUMBNAIL_BLOCK_END';
        const startIndex = fileContent.indexOf(startTag);
        const endIndex = fileContent.indexOf(endTag) + endTag.length;
        if (startIndex === -1 || endIndex === -1) {
            console.log('Thumbnail block tags not found.');
            return fileContent;
        }
        const beforeBlock = fileContent.slice(0, startIndex);
        const afterBlock = fileContent.slice(endIndex);
        return beforeBlock + newThumbnailBlock + afterBlock;
    }
    private async readAndProcessGcodeFile(fileList: Array<any>): Promise<void> {
        const files = Array.from(fileList);
        for (const file of files) {
            if (file.name.endsWith('.gcode')) {
                this.gcodeFileName = file.name;
                return new Promise((resolve, reject) => {
                    const reader = new FileReader();
                    reader.onload = async (e: any) => {
                        const renderFile = files.find(file => file.name === 'Render.png');
                        const thumb = await this.convertImageToThumbnailBlock(renderFile);
                        if (thumb != '') {
                            this.gcodeContent = await this.replaceThumbnailBlock(e.target.result, thumb);
                        }
                        else {
                            this.gcodeContent = e.target.result;
                        }
                        resolve();
                    };
                    reader.onerror = reject;
                    reader.readAsText(file);
                });
            }
        }
    }
    async onFileSelect(event: any): Promise<void> {
        if (event.target.files && event.target.files.length > 0) {
            const files: FileList = event.target.files;
            const filteredFiles: File[] = [];
            for (let i = 0; i < files.length; i++) {
                const file = files[i];
                if (!file.name.toLowerCase().endsWith('.stl')) {
                    filteredFiles.push(file);
                }
            }
            this.imageCount = 0;
            const zip = new JSZip();
            await this.readAndProcessGcodeFile(filteredFiles);
            const totalFiles = filteredFiles.length;
            let processedFiles = 0;
            this.busyUploading = true;
            for (const file of filteredFiles) {
                if ((!file.name.endsWith('.gcode')) && (!file.name.endsWith('Render.png'))) {
                    processedFiles++;
                    const progressPercentage = Math.round((processedFiles / totalFiles) * 100);
                    this.progress = progressPercentage;
                    await this.processImageFile(file, zip);
                }
            }
            if (this.gcodeFileName) {
                zip.file(this.gcodeFileName, this.gcodeContent);
            }
            zip.generateAsync({ type: 'blob' }).then((content) => {
                saveAs(content, this.gcodeFileName.substring(0, this.gcodeFileName.lastIndexOf(".gcode")) + ".zip");
                this.progress = 0;
                this.busyUploading = false;
            });
        }
    }
    private replaceFirstInstanceBetweenDelimiters(content: string, startDelimiter: string, endDelimiter: string, replacementText: string): string {
        const startIndex = content.indexOf(startDelimiter);
        if (startIndex === -1) {
            return content;
        }
        const endIndex = content.indexOf(endDelimiter, startIndex + startDelimiter.length);
        if (endIndex === -1) {
            return content;
        }
        let modifiedContent = content.substring(0, startIndex) + replacementText + content.substring(endIndex + endDelimiter.length);
        return modifiedContent;
    }
    private async drawImageOnCanvas(img: HTMLImageElement, zip: JSZip): Promise<void> {
        const canvas = this.myCanvas.nativeElement;
        const ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
        const stripHeight = 144;
        const numStrips = 14;
        const zipdir = this.gcodeFileName.substring(0, this.gcodeFileName.lastIndexOf(".gcode"));
        const strip = "G1 X@STARTX@ Y@STARTY@ F3570\r\n\
M400\r\n\
@Pause\r\n\
G1 Y@ENDY@ F3570\r\n\
M400";
        for (let i = 0; i < numStrips; i++) {
            const stripY = i * stripHeight;
            const stripCanvas = document.createElement('canvas');
            stripCanvas.width = canvas.width;
            stripCanvas.height = stripHeight;
            const stripCtx = stripCanvas.getContext('2d');
            await stripCtx.drawImage(canvas, 0, stripY, canvas.width, stripHeight, 0, 0, canvas.width, stripHeight);
            if (this.isStripAllWhite(stripCtx, 0, 0, stripCanvas.width, stripHeight)) {
                if (this.gcodeContent != "") {
                    this.gcodeContent = this.replaceFirstInstanceBetweenDelimiters(this.gcodeContent, ";STRIP_" + (i + 1).toString() + "_B", ";STRIP_" + (i + 1).toString() + "_E", "@nothinghere@");
                }
            }
            else {
                await new Promise(resolve => {
                    stripCanvas.toBlob(async (blob) => {
                        zip.file(`${zipdir}/${this.imageCount}.jpg`, blob, { binary: true });
                        this.imageCount++;
                        if (this.gcodeContent != "") {
                            var sss = strip;
                            this.gcodeContent = this.replaceFirstInstanceBetweenDelimiters(this.gcodeContent, ";STRIP_" + (i + 1).toString() + "_B", ";STRIP_" + (i + 1).toString() + "_E", sss);
                        }
                        resolve(undefined);
                    }, 'image/jpeg', 0.99);
                });
            }
        }
    }
    private isStripAllWhite(ctx: CanvasRenderingContext2D, x: number, y: number, width: number, height: number): boolean {
        const imageData = ctx.getImageData(x, y, width, height);
        const data = imageData.data;
        for (let i = 0; i < data.length; i += 4) {
            const r = data[i];
            const g = data[i + 1];
            const b = data[i + 2];
            if ((r < 255 || g < 255 || b < 255)) {
                return false;
            }
        }
        return true;
    }
    private async processImageFile(file: File, zip: JSZip): Promise<void> {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = async (e: any) => {
                const img = new Image();
                img.onload = async () => {
                    await this.drawImageOnCanvas(img, zip);
                    resolve();
                };
                img.onerror = reject;
                img.src = e.target.result;
            };
            reader.onerror = reject;
            reader.readAsDataURL(file);
        });
    }
}
