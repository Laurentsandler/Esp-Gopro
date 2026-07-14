#ESP32 P4 GO-Pro 

## Description:

**Project:** A compact, 3D-printed, battery-powered portable camera built using the Waveshare ESP32 P4 module. 

**Project Motivation:** Initially, the objective was to design a fully custom PCB featuring the new ESP32 P4 chip. However, because the ESP32 P4 is not yet available for JLCPCB assembly, the project pivoted to engineering a custom hardware enclosure and power system around a pre-built Waveshare module. This project addresses the challenge of packaging a bare development board into a functional, portable, and durable camera format.

**Usage Instructions:**

1. Power the device on.
2. The custom flex boot button acts as the primary interactive input (e.g., to trigger a photo or initiate recording).
3. The device can be recharged via the integrated USB-C port.

---

## Images:



* **3D Model Assembly:** <img width="1590" height="914" alt="image" src="https://github.com/user-attachments/assets/6dbca0b0-a093-44dc-a0fd-1f5d39b6a46a" />

* **Case Details:** <img width="2618" height="1104" alt="image" src="https://github.com/user-attachments/assets/e4819c45-e481-4600-a14e-db278e0982d8" />

* **Wiring Diagram:** `[Insert screenshot of the wiring diagram]`


---

## Hardware and CAD

The custom enclosure was entirely designed using Onshape. 

**CAD Files:**

* [Link to Onshape Workspace] https://cad.onshape.com/documents/a52dcf8d564a68f7811ecbec/w/4524fd8eb7e4bb6c92c5f085/e/23b7f0b571128349549216fd?renderMode=0&uiState=6a5633310f2ae7e050e6f1c0]
* `.STEP` files are available in the `/CAD` directory of this repository.

---

## Bill of Materials (BOM)

A comprehensive `bom.csv` is included in this repository. Below is a high-level overview of the primary components:

| Component | Description | Link |
| --- | --- | --- |
| **Microcontroller** | Waveshare ESP32 P4 module with camera connector | `[(https://amzn.eu/d/0bZVOaVJ)]` |
| **Camera** |  camera module  | `[https://fr.aliexpress.com/item/1005004540834095.html?mp=1&pdp_npi=6@dis!USD!USD+7.84!USD+7.01!!USD+7.01!!!@0b88ab6717840344614786601e0fcc!12000029537102320!ct!FR!7032395364!!1!0!&gatewayAdapt=glo2fra]` |
| **Battery** | LiPo Battery | `[ https://fr.aliexpress.com/item/1005009589383276.html?mp=1&pdp_npi=6@dis!USD!USD+20.99!USD+10.50!!USD+10.50!!!@0b88ab6717840345296828381e0fcc!12000049552019935!ct!FR!7032395364!!1!0!&gatewayAdapt=glo2fra]` |
| **Power Management** | Battery charging module | `[https://fr.aliexpress.com/item/1005005037876729.html?mp=1&pdp_npi=6@dis!USD!USD+1.01!USD+0.95!!USD+0.95!!!@0b88ab6717840344614786601e0fcc!12000031413123407!ct!FR!7032395364!!1!0!&gatewayAdapt=glo2fra]` |

---

## Firmware and Software



**Prerequisites:**

* `[Arduino IDE, for flashing the firmware]`
* `[Libraries:

#include <stdio.h> 

#include "esp_log.h" 

#include "esp_sleep.h"

#include "driver/gpio.h"

#include "esp_video.h"

#include "esp_h264_enc.h"

#include "esp_vfs_fat.h"

#include "sdmmc_cmd.h"]`

**Setup Instructions:**

1. `[Step 1: Download the INO file]`
2. `[Step 2: Install required dependencies]`
3. `[Step 3: Compile and flash the firmware to the module]`

---

**Author:** Laurent Sandler

**Total Logged Hours:** 5h+

**Hack Club Funding Level:** `[Insert Level 1/2/3/4 here if applying for the hardware grant]`

**Total Build Cost:** `[Insert $ Amount]`
