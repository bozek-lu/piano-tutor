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
| 037 | Lesson Track (R) | ch 1 | Right-hand channel for built-in lessons on copied SMF Format 0 songs |
| 038 | Lesson Track (L) | ch 2 | Left-hand channel for built-in lessons on copied SMF Format 0 songs |
| 044 | Local Control | On | Keep On so the keyboard still sounds when you play |
| 045 | External Clock | Off | On = Song/Style playback follows incoming MIDI clock |
| 046 | Initial Send | - | Sends the current panel settings to the host as MIDI |
| 061 | Storage Mode | Off | On = keyboard shows up as a USB drive for copying songs (keyboard unplayable while On) |

Checked against the Reference Manual (pages 63-65). The full MIDI implementation chart is in the separate
EZ-310 "Data List" document, not in the Reference Manual.

## EZ-310 MIDI (from the Data List pages 34-36, not yet tested)

Incoming MIDI drives a 16-channel GM/XGlite tone generator and **does not change panel voices or settings**.
Function settings, lesson mode, song/style selection, tempo, transpose and Duo cannot be set over MIDI.

| Message | Received | Sent |
| --- | --- | --- |
| Note on/off | ch 1-16 (lights keys on the 042/043 channels) | yes, velocity 1-127 |
| Program change + Bank Select (CC0/32) | yes, per channel | yes |
| CC1, 7, 10, 64, 72, 82, 91, 93 | yes | yes |
| CC5/65 portamento, 6/38 data entry, 11 expression, 71/73/74 sound, 83/84, 96-101 RPN | yes | no |
| Pitch bend | yes | no |
| CC120/121/123-125 (sound/controllers/notes off), CC122 Local On/Off | yes | no |
| Clock F8, Start FA / Stop FC | yes when External Clock (045) is On | yes, FA/FC when accompaniment starts/stops |
| GM System On `F0 7E 7F 09 01 F7` | resets everything except master tuning | - |
| Master Volume `F0 7F 7F 04 01 ll mm F7` | yes | - |
| Master Tuning `F0 43 1n 27 30 00 00 mm ll cc F7` | yes (also affects panel voices) | - |
| Reverb / Chorus type `F0 43 1n 4C 02 01 00/20 mm ll F7` | yes (also affects panel voices) | - |
| Song Position / Song Select, aftertouch | no | no |
| Active Sensing | no | yes |

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
