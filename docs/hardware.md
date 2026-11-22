# Hardware notes

## Parts

| Part | Notes |
| --- | --- |
| Yamaha EZ-310 | 61 lighted keys, USB TO HOST port (USB-B), USB MIDI + audio |
| M5Stack Tab5 Kit (K145) | ESP32-P4 + ESP32-C6 (Wi-Fi 6), 5" 1280x720 touch, microSD, NP-F550 7.4 V 2000 mAh battery |
| USB-A to USB-B cable, USB 2.0, under 3 m | Tab5 USB-A host -> EZ-310. Yamaha says USB 3.0 cables do not work |
| USB-C charger | Powers and charges the Tab5. Battery only charges while the Tab5 is on |
| microSD card | MIDI songs |

## Wiring

```
EZ-310 [USB TO HOST, USB-B] <--- USB A-B cable ---> [USB-A host] Tab5 [USB-C] <--- charger / PC (flashing)
```

The Tab5 USB-C port is a device / charging port. The keyboard must use the USB-A port.

## EZ-310 settings (from Yamaha manual excerpts, not yet tested)

| Function | Name | Default | Use |
| --- | --- | --- | --- |
| 041 | Light Guide | On | Key lights on/off |
| 042 | Light Part 1 | ch 1 | Incoming MIDI notes on this channel light keys |
| 043 | Light Part 2 | ch 2 | Second light channel (e.g. left hand) |
| 044 | Local Control | On | Keep On so the keyboard still sounds when you play |
| 059 | Storage Mode | Off | On = keyboard shows up as a USB drive for copying songs |

Other notes:

- Songs copied to the keyboard for its built-in lessons should be **SMF Format 0**
  (EZ-300 owners report Format 1 plays but has no lesson functions).
- Open question: does a lit note also sound? If yes, try sending CC7 (volume) = 0 on the light channel.

## Known risks

- **USB enumeration:** the EZ-310 is an audio + MIDI composite device; the MIDI library has not been
  tested with Yamaha keyboards.
- **Tab5 USB host bug:** full-speed devices failed to enumerate with IDF 5.3 / M5Stack Arduino fork
  (m5stack/M5Stack#344, reportedly fixed in ESP-IDF 5.4.2). Build with ESP-IDF >= 5.4.2.

## References

- EZ-310 Reference Manual: https://data.yamaha.com/files/download/other_assets/2/2295422/EZ310_reference_manual_En_B0.pdf
- Tab5 docs: https://docs.m5stack.com/en/core/Tab5
- ESP32_Host_MIDI (USB host MIDI, lists ESP32-P4): https://github.com/sauloverissimo/ESP32_Host_MIDI
- Espressif Tab5 USB host example: https://components.espressif.com/components/espp/usb_host/examples/tab5_example
