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

 **Wiring Diagram:** <img width="2458" height="1412" alt="image" src="https://github.com/user-attachments/assets/9b1bf089-3414-4471-bdf2-81364a377213" />


---

## Bill of Materials (BOM)

A complete `bom.csv` is included in the repository (`docs/BOM.csv`). Primary components include:

| Component | Description | Qty | Price (USD) | Source |
| :--- | :--- | :--- | :--- | :--- |
| **Microcontroller** | Waveshare ESP32-P4 Module (w/ camera connector) | 1 | ~$21.89 | [Amazon](https://amzn.eu/d/0bZVOaVJ) |
| **Camera** | Compatible Camera Module | 1 | ~$7.01 | [AliExpress](https://fr.aliexpress.com/item/1005004540834095.html) |
| **Battery** | LiPo Battery | 1 | ~$10.50 | [AliExpress](https://fr.aliexpress.com/item/1005009589383276.html) |
| **Power Management** | Battery Charging Module | 1 | ~$0.95 | [AliExpress](https://fr.aliexpress.com/item/1005005037876729.html) |





---

## Firmware & Software

The firmware targets the ESP-IDF framework directly (not Arduino — ESP32-P4 Arduino/PlatformIO-Arduino support is still unstable, Espressif itself recommends ESP-IDF for this chip at this stage). It's built with [PlatformIO](https://platformio.org/), which wraps ESP-IDF's build system for you.

**Prerequisites:**
* [PlatformIO](https://platformio.org/install) (CLI or the VS Code extension)
* That's it locally — PlatformIO downloads the ESP-IDF toolchain itself on first build.

**Dependencies:**
This project uses Espressif's official camera/video stack for the ESP32-P4, declared in [`src/idf_component.yml`](Firmware/src/idf_component.yml) and pulled automatically by the IDF Component Manager on first build (no manual download needed):
* [`espressif/esp_video`](https://components.espressif.com/components/espressif/esp_video) — V4L2-compatible camera driver framework for the ESP32-P4's MIPI-CSI interface. Transitively brings in `esp_cam_sensor` (OV5647 sensor driver), `esp_ipa` (image processing/AE/AWB), and `esp_sccb_intf` (camera control bus).
* [`espressif/esp_h264`](https://components.espressif.com/components/espressif/esp_h264) — hardware-accelerated H.264 encoder, used to encode video on the fly while recording.
* Everything else (FATFS, SDMMC host driver, GPIO, deep sleep) ships as part of ESP-IDF itself — no extra install.

**Building & flashing:**
```bash
cd Firmware
pio run              # build
pio run -t upload    # flash over USB-C
pio device monitor    # serial log at 115200 baud
```

**How it works:** the board deep-sleeps between button presses to save battery. A single click on BOOT starts recording (click again to stop); three clicks within 800 ms takes a photo. See the comment block at the top of `src/main.cpp` for the handful of hardware-specific details (exact button GPIO, deep-sleep wakeup API) worth double-checking against your specific board revision before you rely on it.
