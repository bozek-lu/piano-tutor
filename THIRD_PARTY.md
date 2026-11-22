# Third-party code

## M5MusicTab5

https://github.com/Fangbrbr/M5MusicTab5 (commit 79e752b, 2026-11-14), MIT License,
Copyright (c) 2026 Fangbrbr. Full licence text: `firmware/components/midi_file/LICENSE-M5MusicTab5`.

| Here | From | Changes |
| --- | --- | --- |
| `firmware/components/midi_file/engine_midi_smf.c`, `include/engine_midi_smf.h` | `components/engine_midi/` | none |
| `firmware/components/midi_file/include/engine_midi_config.h` | `components/engine_midi/include/` | trimmed to the SMF constant |
| `firmware/components/usb_midi_host/` | `components/service_usb_host/service_usb_host.c` | MIDI only (HID removed), MIDI OUT endpoint and send API, callback API, own client task, unplug handling |
| `firmware/sdkconfig.defaults` | `sdkconfig` | Tab5 chip, flash and PSRAM settings |
