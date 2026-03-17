# USBtoDB9_4Amiga
A solution to use a specific USB joystick - Retrogames ltd 'The Joystick - the clear one' on a DB9 port on an Amiga 500 using a Pi Pico W. I wanted to use USB inputs, to pull the DB9 pins to LOW so that the Amiga would recognise the USB inputs. 

<img width="500" height="545" alt="image" src="https://github.com/user-attachments/assets/666811bb-e1f7-4d71-9419-1e1be0a15cc2" />

First thing, A reference to the pinout on the Amiga DB9

| Pin | Function | Direction | Notes |
|-----|----------|-----------|-------|
| 1 | Up | Output | Active low signal |
| 2 | Down | Output | Active low signal |
| 3 | Left | Output | Active low signal |
| 4 | Right | Output | Active low signal |
| 5 | Unused | - | Leave unconnected |
| 6 | Fire 1 | Output | Active low signal |
| 7 | +5V | Power | Powers the adapter |
| 8 | Ground | Power | Common ground |
| 9 | Fire 2 | Output | Active low signal |

> **Note:** All signal pins are active low — the Amiga detects a joystick input when a pin is pulled to ground. The Amiga supplies +5V on pin 7 which powers the Pico W adapter directly via VBUS (pin 40). Don't make the mistake that i did initially of powering the Pico via VSYS - This will not work or power the joystick, when using USBHost mode with the Pico, we need to power the Pico at VBUS.

I used a multimeter to trace continuity from my DB9 to the 9 outcoming cables, and recoded colours to pin number. 

## DB9 Cable Wire Colours

| Pin | Function | Wire Colour |
|-----|----------|-------------|
| 1 | Up | Red |
| 2 | Down | Black |
| 3 | Left | Grey |
| 4 | Right | Orange |
| 5 | Unused | Brown |
| 6 | Fire 1 | Green |
| 7 | +5V Power | White |
| 8 | Ground | Blue |
| 9 | Fire 2 | Yellow |

> **Note:** Always verify wire colours on your specific cable using a multimeter in continuity mode before soldering. Do not assume wire colours match this table.

![IMG_20260313_115015033](https://github.com/user-attachments/assets/d5bf6189-fd05-4dd4-add7-e05779073d2a)

Following this i soldered the DB9 to the Pico, 
The following table shows the wire connections from the DB9 cable to the Raspberry Pi Pico W GPIO pins. On purpose i chose pins that were a little further apart to help with my soldering. 

| Wire Colour | DB9 Pin | Function | Pico W Pin |
|-------------|---------|----------|------------|
| Red | 1 | Up | GP2 |
| Black | 2 | Down | GP4 |
| Grey | 3 | Left | GP6 |
| Orange | 4 | Right | GP8 |
| Green | 6 | Fire 1 | GP10 |
| Yellow | 9 | Fire 2 | GP12 |
| White | 7 | +5V Power | VBUS (Pin 40) |
| Blue | 8 | Ground | GND (Pin 38) |
| Brown | 5 | Unused | Not connected |

> **Note:** The brown wire (DB9 pin 5) is unused. Insulate the bare end with electrical tape to prevent accidental shorts. The image also shows me powering VSYS instead of VBUS - VSYS does not work i later learnt. 


## Arduino IDE Setup

Install Adruino IDE to program the Pi Pico. 

### Board Support

We need to add RP2040 Pi support to Arduino using the following Git. 

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add the following URL to **Additional Boards Manager URLs**:
```
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```
4. Go to **Tools → Board → Boards Manager**
5. Search for **RP2040**
6. Install **Raspberry Pi Pico/RP2040 by Earle Philhower**

### Board Selection
- Go to **Tools → Board → Raspberry Pi Pico W**

### USB Stack Selection
- Go to **Tools → USB Stack → Adafruit TinyUSB Host**

### Required Libraries
Install the following libraries via **Tools → Manage Libraries**:

| Library | Author | Notes |
|---------|--------|-------|
| Adafruit TinyUSB Library | Adafruit | Install with all dependencies when prompted |

### Additional File Required
The sketch requires a `tusb_config.h` file in the same sketch folder as the main `.ino` file containing the following:
```c
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_RHPORT0_MODE     OPT_MODE_HOST

#endif
```

> **Note:** This file is included in the repository and must remain in the same folder as the main sketch file.



## Firmware Overview

The firmware is written in C++ using the Arduino IDE framework and the Adafruit TinyUSB library.

### How it works

On boot the Pico W initialises all GPIO pins as outputs and sets them HIGH (released). It then starts the TinyUSB Host stack which monitors the USB port for incoming devices.

When a USB joystick or gamepad is connected via a USB>USB-C adapter (Directly to the on-board Pico USB-C), TinyUSB detects it as a HID device and begins receiving reports continuously at approximately 8-10ms intervals. It took a long time to work out how to get the PICO listening on the on board USB-C. Lots of repo's out there helped finally work it out, including the Tiny USB host examples - hid_controller repo 
```
https://github.com/hathach/tinyusb/tree/master/examples/host/hid_controller
```
Each report contains the current state of all axes and buttons. The firmware reads three values from each report:

- **Byte 0** — X axis (left/right direction)
- **Byte 1** — Y axis (up/down direction)  
- **Byte 5** — Button states (fire buttons)

> **Note:**  Byte 5 was found to be correct for this joystick, it doesn't necessarily match any other joysticks or peripherals.

These values are translated to GPIO pin states:

- If the X axis value is below 0x40 — LEFT pin pulled LOW (pressed)
- If the X axis value is above 0xC0 — RIGHT pin pulled LOW (pressed)
- If the Y axis value is below 0x40 — UP pin pulled LOW (pressed)
- If the Y axis value is above 0xC0 — DOWN pin pulled LOW (pressed)
- If button byte has 0x20 bit set — FIRE1 pin pulled LOW (pressed)
- If button byte has 0x10 bit set — FIRE2 pin pulled LOW (pressed)

When a pin is pulled LOW the Amiga detects it as a joystick input — identical to how an original joystick works.

When the joystick is disconnected all pins are released HIGH and the adapter waits for a new device to connect.

### LED Indicators

| LED State | Meaning |
|-----------|---------|
| Solid | Powered up, waiting for joystick |
| Solid (after connect) | Joystick connected and receiving data |

> **Note:** Button byte values (0x10 for Fire 1 and 0x20 for Fire 2) are specific to the Retrogames THEJoystick. Other USB controllers may use different values and may require remapping in the firmware.

Install the firmware on the PICO - Using Adruino IDE with all the settings above, open the ino and h file into a single sketch - connect and upload to Pico. Other than a few issues to untangle with Fire2 being intermittent (which may be my solder job) we have a fully functional, brand new USB joystick working on a classic Amiga 500.



