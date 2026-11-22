# Tab5 firmware

ESP-IDF app for the M5Stack Tab5 (ESP32-P4).

Current test firmware (milestones 1-2):

1. Switches on 5 V for the USB-A port and waits for a USB MIDI device.
2. Logs every key you press on the EZ-310 to the serial monitor.
3. When the keyboard connects, lights a C major scale (C4-C5) on channel 1, the EZ-310's
   default Light Part 1. If the keys light up, the tutor can work.

`components/midi_file` (SMF parser) is built in but not used yet: it is for loading songs from
the microSD card next.

## Build and flash

Install ESP-IDF **5.4.2 or newer** (CI uses 5.5.5; Windows: Espressif's ESP-IDF installer),
then in an ESP-IDF terminal:

```bash
cd firmware
idf.py set-target esp32p4
idf.py build
idf.py -p COM5 flash monitor   # use the COM port the Tab5 shows up on
```

Flash and watch logs through the Tab5 **USB-C** port. The keyboard goes in the **USB-A** port.

If the keyboard is not detected, the log says why: "not a USB MIDI device" (no MIDI Streaming
interface), enumeration errors (descriptor too large: raise `CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE`),
or "no MIDI OUT endpoint" (keys can be read but not lit).
