/*
 * SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
 * SPDX-License-Identifier: LicenseRef-NCL-1.0
 * 
 * Original CMY Labs PTY LTD content, licensed under NCL-1.0 ONLY.
 * Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.
 */
import { AfterViewInit,Component,ElementRef,ViewChild } from '@angular/core';
import { CommandService } from '../command.service';
import { PrinterCommand,PrinterCommandType } from '../printer';
import { SettingsModalService } from '../settings-modal.service';
@Component({
    selector: 'app-image-display',
    templateUrl: './image-display.component.html',
    styleUrls: ['./image-display.component.scss']
})
export class ImageDisplayComponent implements AfterViewInit {
    @ViewChild('imageCanvas', { static: false })
    canvasRef: ElementRef;
    @ViewChild('topLeft')
    topLeft: ElementRef<HTMLDivElement>;
    @ViewChild('topRight')
    topRight: ElementRef<HTMLDivElement>;
    @ViewChild('bottomLeft')
    bottomLeft: ElementRef<HTMLDivElement>;
    @ViewChild('bottomRight')
    bottomRight: ElementRef<HTMLDivElement>;
    private ctx: CanvasRenderingContext2D;
    private backBufferCanvas: HTMLCanvasElement;
    private backBufferCtx: CanvasRenderingContext2D;
    public squareSize = 50;
    public squareX = 375;
    public squareY = 275;
    private image = new Image();
    private isMoving = false;
    private isRotating = false;
    private isScaling = false;
    private initialScaleLength: number = 0;
    public gridDensity: number = 50;
    public scaleFactor: number = 1.1;
    scaledWidth = 1000;
    scaledHeight = 800;
    public rotationAngle: number = 0;
    private currentRotation: number = 0;
    private firstButtonSquareX: number;
    private firstButtonSquareY: number;
    private firstButtonSquareSize: number;
    private secondButtonSquareX: number;
    private secondButtonSquareY: number;
    private secondButtonSquareSize: number;
    public offsetXmm: number = 0;
    public offsetYmm: number = 0;
    constructor(private settingsModalService: SettingsModalService, private commandService: CommandService) {
    }
    sendMove(_dir: string) {
        let aligncommand: PrinterCommand;
        aligncommand = {
            id: PrinterCommandType.Move,
            command: PrinterCommandType.Move,
            parameters: {
                x: this.offsetXmm
            }
        };
        this.commandService.sendCommand(aligncommand);
        aligncommand = {
            id: PrinterCommandType.Move,
            command: PrinterCommandType.Move,
            parameters: {
                y: this.offsetYmm
            }
        };
        this.commandService.sendCommand(aligncommand);
    }
    setOff() {
        this.settingsModalService.setOffsetOpen(false);
    }
    ngAfterViewInit() {
        this.ctx = this.canvasRef.nativeElement.getContext('2d');
        this.backBufferCanvas = document.createElement('canvas');
        this.backBufferCanvas.width = this.canvasRef.nativeElement.width;
        this.backBufferCanvas.height = this.canvasRef.nativeElement.height;
        this.backBufferCtx = this.backBufferCanvas.getContext('2d');
        this.backBufferCtx.drawImage(this.image, 0, 0);
    }
    public calculateOffset2() {
        const centerFirstSquareX = this.firstButtonSquareX + this.firstButtonSquareSize / 2;
        const centerFirstSquareY = this.firstButtonSquareY + this.firstButtonSquareSize / 2;
        const centerSecondSquareX = this.secondButtonSquareX + this.secondButtonSquareSize / 2;
        const centerSecondSquareY = this.secondButtonSquareY + this.secondButtonSquareSize / 2;
        const offsetXpx = centerSecondSquareX - centerFirstSquareX;
        const offsetYpx = centerSecondSquareY - centerFirstSquareY;
        const pixelToMmRatio = 60 / this.firstButtonSquareSize;
        this.offsetXmm = offsetXpx * pixelToMmRatio * -1;
        this.offsetYmm = offsetYpx * pixelToMmRatio * -1;
    }
    public storePropertiesForFirstButton() {
        this.firstButtonSquareX = this.squareX;
        this.firstButtonSquareY = this.squareY;
        this.firstButtonSquareSize = this.squareSize;
        const scaleFactor = this.squareSize / 63.3;
        const scaledSq2: any = {
            width: 20 * scaleFactor,
            height: 20 * scaleFactor,
        };
        this.squareSize = scaledSq2.width;
        this.UpdateFrame();
    }
    public storePropertiesForSecondButton() {
        this.secondButtonSquareX = this.squareX;
        this.secondButtonSquareY = this.squareY;
        this.secondButtonSquareSize = this.squareSize;
    }
    private drawGrid() {
        const canvasWidth = 1000;
        const canvasHeight = 800;
        this.ctx.strokeStyle = 'rgba(0,0,255,0.5)';
        this.ctx.lineWidth = 1;
        for (let x = 0; x <= canvasWidth; x += this.gridDensity) {
            this.ctx.beginPath();
            this.ctx.moveTo(x, 0);
            this.ctx.lineTo(x, canvasHeight);
            this.ctx.stroke();
        }
        for (let y = 0; y <= canvasHeight; y += this.gridDensity) {
            this.ctx.beginPath();
            this.ctx.moveTo(0, y);
            this.ctx.lineTo(canvasWidth, y);
            this.ctx.stroke();
        }
    }
    onImageSelected(event: Event) {
        const file = (event.target as HTMLInputElement).files[0];
        this.image.src = '';
        if (file) {
            const reader = new FileReader();
            reader.onload = (e: any) => {
                this.image.src = e.target.result;
                this.image.onload = () => {
                    this.canvasRef.nativeElement.width = 1000;
                    this.canvasRef.nativeElement.height = 800;
                    const canvasAspectRatio = 1000 / 800;
                    const imageAspectRatio = this.image.width / this.image.height;
                    if (imageAspectRatio > canvasAspectRatio) {
                        this.scaledWidth = 1000;
                        this.scaledHeight = this.scaledWidth / imageAspectRatio;
                    }
                    else {
                        this.scaledHeight = 800;
                        this.scaledWidth = this.scaledHeight * imageAspectRatio;
                    }
                    this.backBufferCtx.clearRect(0, 0, 1000, 800);
                    this.backBufferCtx.drawImage(this.image, 0, 0, this.scaledWidth, this.scaledHeight);
                    this.backBufferCtx.fillRect(this.scaledWidth, this.scaledHeight, 1000 - this.scaledWidth, 800 - this.scaledHeight);
                    this.convertToGrayscaleAndThreshold(this.image);
                    this.DrawFrame();
                    this.UpdateFrame();
                };
            };
            reader.readAsDataURL(file);
        }
    }
    convertToGrayscaleAndThreshold(_image: HTMLImageElement) {
        if (isNaN(this.scaledWidth) || isNaN(this.scaledHeight))
            return;
        const imageData = this.backBufferCtx.getImageData(0, 0, 1000, 800);
        this.ctx.putImageData(imageData, 0, 0);
        this.drawGrid();
        this.placeCorners();
        this.drawSquare();
    }
    placeCorners() {
        const canvasRect = this.canvasRef.nativeElement.getBoundingClientRect();
        const corners = [
            { el: this.topRight, x: this.squareX + this.squareSize + 8, y: this.squareY + canvasRect.top - 30 },
            { el: this.bottomLeft, x: this.squareX - 3, y: this.squareY + this.squareSize + canvasRect.top - 10 },
            { el: this.bottomRight, x: this.squareX + this.squareSize + 3, y: this.squareY + this.squareSize + canvasRect.top - 10 },
        ];
        corners.forEach(corner => {
            corner.el.nativeElement.style.left = `${corner.x - 5}px`;
            corner.el.nativeElement.style.top = `${corner.y - 5}px`;
        });
    }
    onMouseDown2(event: MouseEvent, action?: string) {
        const rect = this.canvasRef.nativeElement.getBoundingClientRect();
        if (action === 'rotate') {
            this.isRotating = true;
            const centerX = this.squareX + this.squareSize / 2;
            const centerY = this.squareY + this.squareSize / 2;
            const dx = event.clientX - rect.left - centerX;
            const dy = event.clientY - rect.top - centerY;
            this.rotationAngle = Math.atan2(dy, dx);
        }
        else if (action === 'scale') {
            this.isScaling = true;
            const centerX = this.squareX + this.squareSize / 2;
            const centerY = this.squareY + this.squareSize / 2;
            const dx = event.clientX - rect.left - centerX;
            const dy = event.clientY - rect.top - centerY;
            this.initialScaleLength = Math.sqrt(dx * dx + dy * dy);
        }
        else {
            this.isMoving = true;
        }
    }
    onMouseMove2(event: MouseEvent) {
        const rect = this.canvasRef.nativeElement.getBoundingClientRect();
        if (this.isMoving) {
            const rect = this.canvasRef.nativeElement.getBoundingClientRect();
            this.squareX = event.clientX - rect.left + 8;
            this.squareY = event.clientY - rect.top - this.squareSize - 8;
            this.drawSquare();
            this.placeCorners();
            this.UpdateFrame();
        }
        else if (this.isRotating) {
            const centerX = this.squareX + this.squareSize / 2;
            const centerY = this.squareY + this.squareSize / 2;
            const dx = event.clientX - rect.left - centerX;
            const dy = event.clientY - rect.top - centerY;
            const angle = Math.atan2(dy, dx);
            const rotation = angle - this.rotationAngle;
            this.currentRotation = rotation;
            this.drawSquare();
            this.placeCorners();
            this.UpdateFrame();
        }
        else if (this.isScaling) {
            const centerX = this.squareX + this.squareSize / 2;
            const centerY = this.squareY + this.squareSize / 2;
            const dx = event.clientX - rect.left - centerX;
            const dy = event.clientY - rect.top - centerY;
            const currentLength = Math.sqrt(dx * dx + dy * dy);
            const scaleFactor = currentLength / this.initialScaleLength;
            this.squareSize *= scaleFactor;
            this.initialScaleLength = currentLength;
            this.drawSquare();
            this.placeCorners();
            this.UpdateFrame();
        }
    }
    onMouseUp2(_event: MouseEvent) {
        this.isMoving = false;
        this.isRotating = false;
        this.isScaling = false;
    }
    public get displayRotationAngle(): number {
        return this.currentRotation * (180 / Math.PI);
    }
    public set displayRotationAngle(angle: number) {
        this.currentRotation = angle * (Math.PI / 180);
    }
    private drawSquare() {
        this.ctx.fillStyle = 'rgba(255,0,0,0.25)';
        this.ctx.fillRect(this.squareX, this.squareY, this.squareSize, this.squareSize);
    }
    public DrawFrame() {
        const centerX = this.canvasRef.nativeElement.width / 2;
        const centerY = this.canvasRef.nativeElement.height / 2;
        this.ctx.clearRect(0, 0, 1000, 800);
        this.ctx.translate(centerX, centerY);
        this.ctx.rotate(this.currentRotation);
        this.ctx.drawImage(this.image, -centerX, -centerY, this.scaledWidth, this.scaledHeight);
        this.ctx.setTransform(1, 0, 0, 1, 0, 0);
        this.drawGrid();
        this.drawSquare();
    }
    public UpdateFrame() {
        const centerX = this.canvasRef.nativeElement.width / 2;
        const centerY = this.canvasRef.nativeElement.height / 2;
        this.backBufferCtx.clearRect(0, 0, 1000, 800);
        this.backBufferCtx.translate(centerX, centerY);
        this.backBufferCtx.rotate(this.currentRotation);
        this.backBufferCtx.drawImage(this.image, -centerX, -centerY, this.scaledWidth, this.scaledHeight);
        this.backBufferCtx.setTransform(1, 0, 0, 1, 0, 0);
        this.convertToGrayscaleAndThreshold(this.image);
    }
    public applyGrid() {
        this.UpdateFrame();
    }
}
