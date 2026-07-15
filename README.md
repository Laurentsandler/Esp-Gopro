# ESP32-P4 GO-Pro
<img width="1564" height="1006" alt="image" src="https://github.com/user-attachments/assets/96ae182b-3dbd-4d3a-afa1-bb431c775fef" />

## Overview

A compact, 3D-printed, battery-powered portable camera based on the Waveshare ESP32-P4 module.

**Motivation:**
I wanted to build a small action camera that can record me biking, among other cool stuff. I initally wanted to make a custom pcb, but the esp32 p4 is not yet available for pcba assembly.

## Usage

1. Connect the battery to the charger board, and connect the charger board to the mainboar.
2. Press the flex boot button to capture photos or start/stop recording.
3. Recharge using the USB-C port(on the charger board).

---

## Hardware & CAD

The custom enclosure was designed in Onshape. 

* **CAD Files:** https://cad.onshape.com/documents/a52dcf8d564a68f7811ecbec/w/4524fd8eb7e4bb6c92c5f085/e/23b7f0b571128349549216fd?renderMode=0&uiState=6a5750859237108cff857e8d


### Images

 **3D Model Assembly:** <img width="1590" height="914" alt="image" src="https://github.com/user-attachments/assets/6dbca0b0-a093-44dc-a0fd-1f5d39b6a46a" />

 **Case Details:** <img width="2618" height="1104" alt="image" src="https://github.com/user-attachments/assets/e4819c45-e481-4600-a14e-db278e0982d8" />

 **Wiring Diagram:** *[Insert screenshot of the wiring diagram]*

---

## Bill of Materials (BOM)

A complete `bom.csv` is included in the repository. Primary components include:

| Component | Description | Source |
| :--- | :--- | :--- |
| **Microcontroller** | Waveshare ESP32-P4 Module (w/ camera connector) | [Amazon](https://amzn.eu/d/0bZVOaVJ) |
| **Camera** | Compatible Camera Module | [AliExpress](https://fr.aliexpress.com/item/1005004540834095.html) |
| **Battery** | LiPo Battery | [AliExpress](https://fr.aliexpress.com/item/1005009589383276.html) |
| **Power Management** | Battery Charging Module | [AliExpress](https://fr.aliexpress.com/item/1005005037876729.html) |



---

## Firmware & Software

**Prerequisites:**
* Arduino IDE (for compiling and flashing the firmware)
* Required C headers/dependencies:
