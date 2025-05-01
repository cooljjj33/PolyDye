# SD Card Flash Files

This folder contains the necessary files to update your unit. Follow the steps below to ensure a successful update process.

This folder contains 2 versions:
1. /LookAhead: Implements a lookahead to accomodate prints not starting due to invalid Marlin ok responses. Uses YOhY Alignment and has improved image clarity on flat vertical surfaces.
2. /NoLookAhead: This is the previously released version(1.15) that some users reported issues with but functional for others. Waits for Marlin Ok before continuing, Does not use YohY alignment. May produce vertical wall artefacts.

Please only flash the contents of one folder to your unit as not to overwrite previous version.

## How to Use

1. **Prepare an SD Card**
   - Ensure the SD card is formatted as FAT32.

2. **Copy Files**
   - Copy **all the contents** of this folder to the **root directory** of your SD card.  
     *Do not place the files in a subfolder.*

3. **Insert the SD Card**
   - Insert the SD card into the unit's designated SD card slot and power on the unit.

4. **Flash Process**
   - A light on the unit will turn **ON**, indicating the flash process has started.
   - Wait patiently. Do **not remove** the SD card while the light is on.

5. **Completion**
   - The light will turn **OFF** once the flashing process is complete.
   - You may now safely remove the SD card.

## Important Notes

- **Do not interrupt the flashing process.** Removing the SD card or powering off the unit prematurely may cause unexpected behavior or damage the unit.


