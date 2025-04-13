# PolyDye Full Color 3D Printer Mod

![PolyDye Full Color 3D Printer Mod](https://www.level9000.co.za/images/PolyDyeHeading.jpg)

The **PolyDye Full Color 3D Printer Mod** brings vibrant, full-color printing to Marlin-based printers by integrating inkjet printing technology directly into your 3D printer workflow. This modification applies ink layers between white filament layers to color your model.

⚠️ **Disclaimer:** The software is currently in **beta**, and this repository is **not yet complete**. Expect frequent updates and experimental features. We will be adding the rest of the files over the next month or 3...

## Disclaimer

This project is provided "as is" without warranty of any kind, either express or implied. The author is not liable for any damages resulting from the use of this project.

---

## ChangeLog

Due to the recent opacity changes I introduced an issue where one of the processors were starved casuing some issues including causing the print to fail right after G29. With this build I address this. If you level with every print and have a G29 in your start gcode, please add the following line below the G29

;waitforlevel

This should allow the print to continue after level. It has a side effect where the Y Offset needs to be adjusted by 5.9mm to get it in the correct place again.

[v1.15] – 2025-04-10

Fixed

    Alignment errors are now minimized and should not occur as frequently

Changed

    YOhY Hot buttons have been removed. 

    XY Hot alignment up and down buttons have been inverted so that the up button moves the ink towards the back of the bed and the down button moves the ink to the front of the bed. 

Known Issues

    The unit must be power-cycled before each print and after stopping an unfinished print, or it may continue sending G-code and risk damaging the printer.

    Intermittent freezing: The print may randomly stop during operation. Monitor your prints and avoid leaving the unit unattended.

Notes

    This release is for Marlin firmware only.

    A Klipper build is in progress and will be released in a few days pending a fix.


[v1.14] – 2025-03-30

Added

    Improved color performance on more opaque filaments.
    (Tested on Creality Hyper Series White and CR-PLA White(best))

    "Auto Apply" button to automatically apply and save offsets after clicking Store Ink and Calculate.

    Enhanced fine detail printing for sharper and more accurate output.

Changed

    Nozzles now fire more reliably and consistently.

    OrcaSlicer thumbnails are no longer required.

    XY alignment process simplified:

        Second square is now automatically scaled for ink square alignment.

        Only positioning is needed after storing filament.

        Offset application is streamlined with the new Auto Apply button.

Fixed

    Corrected rounding artifacts in color bands around low-poly square geometry.

Removed

    YohY alignment system:

        Hot calibration buttons for YohY have been deprecated.

        Users must now manually set the YohY alignment value to 184000 in settings and not change it.

Known Issues

    The unit must be power-cycled before each print and after stopping an unfinished print, or it may continue sending G-code and risk damaging the printer.

    Intermittent freezing: The print may randomly stop during operation. Monitor your prints and avoid leaving the unit unattended.

Notes

    This release is for Marlin firmware only.

    A Klipper build is in progress and will be released in a few days pending a fix.

---
	
## Quick Links

- **[Assembly Instructions](https://www.level9000.co.za/How-To.html)**  
  Step-by-step guide to install and integrate the PolyDye mod into your 3D printer.

- **[Calibration Guide](https://www.level9000.co.za/Calibration.html)**  
  Detailed instructions for calibrating your printer for full-color prints.

- **[Hardware Requirements](https://www.level90003dprintedmodels.com/shop)**  
  A list of required parts and where to obtain them.

---

## Features

- Adds full-color capabilities to any Marlin compatible 3D printer.
- Ink application synchronized with white filament layers.
- Integration with ESP32-S3 for precise color control.

---

## Credits

This project would not have been possible without the contributions of the following individuals and organizations:

1. **Jeroen Domburg** - For his hack on HP 803 inkjet cartridges where he provided pinouts and sample code that served as a foundation for this mod. A portion of the sales go to his tip jar. 
2. **Adafruit** - Their research and work on hacking the LCD interface of the ESP32-S3 interface.  
3. **Bianca Mariani** - Thanks to my loving wife for putting up with me during the four years of developing this mod, editing the videos and soldering tiny components when I couldn't see anymore.

---

Stay tuned for updates as we continue to refine and expand this exciting project!
