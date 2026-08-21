---

title: "DIY ESP32-P4 GoPro"
author: "Laurent Sandler"
description: "Development journal for a DIY ESP32-P4 GoPro"
created_at: "2026-06-19"
------------------------

# June 19: Created the concept and picked parts

Why the ESP32-P4 at all

I picked the P4 over the ESP32-S3 because I needed the ability to record video at a usable frame rate and quality, this requires a hardware video encoder and well as MIPI_CSI input, This is enough for video at 1080p30. The tradeoff i'm going to have the accept is no native wifi support without a sister ESP32-C6 chip.

The custom PCB endeavor

My first plan was a custom PCB with a bare ESP32-P4 module, so I could put the camera connector, charger, and battery management on a single board sized exactly to the case. I spent about ~10 minutes in JLCPCB's parts library searching, before I found out that that the ESP32-P4 isn't stocked for assembly yet.
and dealing with soldering under pcb pins is a headache, so I had to resort to a prebuilt module.

I found this Waveshare ESP32-P4 module with a camera connector.

<img width="1622" height="722" alt="Waveshare ESP32-P4 module" src="https://github.com/user-attachments/assets/c0969340-c764-4e42-912f-5dd5786fe05e" />

Finding a suitable camera was trickier, as this board does not have a standard OV5640 connector, it uses one similar to the one on a raspberry pi 02w. So I needed a compatible camera.

<img width="1622" height="722" alt="Camera" src="https://github.com/user-attachments/assets/002ce065-a99e-400f-85c5-32607f43d231" />

For the battery, I wanted it to fit directly behind the footprint of the main board. I found one that should give me approximately 1.5–2 hours of runtime at 1080p @ 30 FPS.

<img width="1098" height="462" alt="Battery" src="https://github.com/user-attachments/assets/691d7692-a0e3-4644-bc9e-538ce9cd966d" />

I also needed a battery charging module.

<img width="1107" height="450" alt="Battery charging module" src="https://github.com/user-attachments/assets/8e4354a2-8058-4469-91e0-7d6dcd91a685" />

**Total time spent:** 1 hour

# June 19: Designed the case

I designed the case in Onshape, with flex buttons for the reset and boot buttons. The boot button will serve as the only interactive button. I also added holes for the onboard microphone, camera, and USB-C charging port, as well as a snap-on lid. Rather than mounting physical switches, I made the reset and boot buttons as hinge flexures in the case wall, which are thin sections of plastic that bend enough to press the board's onboard tactile switches.

The boot button doubles as the only user control: start/stop recording, and to take photos. Reset stays the same. The chip is kept in a low power state until the button is pressed to start recording or too take a photo.

<img width="1590" height="914" alt="Case design" src="https://github.com/user-attachments/assets/4b4dc32f-6a65-48c8-b79e-e28a60ff42f3" />

<img width="1590" height="914" alt="Case design" src="https://github.com/user-attachments/assets/74ecd056-443d-4e5f-af45-cf0e833cb9e6" />

<img width="1590" height="914" alt="Case design" src="https://github.com/user-attachments/assets/027984c9-1c15-415f-b6ad-f1f61d5ab74b" />

<img width="2618" height="1104" alt="Case design" src="https://github.com/user-attachments/assets/79423964-6ca7-48f0-b6fa-6858ac9d1b04" />

**Total time spent:** 4.5 hours

# August 7: Created the Git repository

I spent some time documenting everything about the project and build inside the README file in the repository. I also uploaded all of the files from Onshape, as well as the code. I created a BOM and drew a wiring diagram.

<img width="1419" height="1658" alt="Wiring diagram" src="https://github.com/user-attachments/assets/5f8deb48-9dab-4a58-b49d-222cd1229469" />

<img width="868" height="1709" alt="Project files" src="https://github.com/user-attachments/assets/4f24de7d-0dc5-4635-8aa5-c6f3f99b1f17" />

**Total time spent:** 2.5 hours
