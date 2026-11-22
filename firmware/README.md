# Tab5 firmware

ESP-IDF app for the M5Stack Tab5 (ESP32-P4). Currently a skeleton that boots and logs.

## Build and flash

Install ESP-IDF **5.4.2 or newer** (Windows: Espressif's ESP-IDF installer), then in an
ESP-IDF terminal:

```bash
cd firmware
idf.py set-target esp32p4
idf.py build
idf.py -p COM5 flash monitor   # use the COM port the Tab5 shows up on
```

Flash through the Tab5 **USB-C** port. The keyboard goes in the **USB-A** port.

CI (`.github/workflows/firmware.yml`) builds the same project on every push.
