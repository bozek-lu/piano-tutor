# piano-tutor: notes for coding agents

Flowkey-style practice helper: an M5Stack Tab5 (ESP32-P4) drives the key lights of a Yamaha EZ-310
over USB MIDI. Start with `README.md` (plan) and `docs/hardware.md` (wiring, EZ-310 settings, risks).

## Setting up a machine

Don't install tools by hand. Run the script for the OS; it's safe to re-run and finishes with a firmware build:

- macOS: `./scripts/setup-mac.sh` (needs Homebrew and Xcode command line tools; ESP-IDF goes to `~/esp/esp-idf-v5.5.5`)
- Windows: `powershell -ExecutionPolicy Bypass -File scripts\setup-windows.ps1` (winget; ESP-IDF via Espressif's EIM: source in `C:\esp\v5.5.5\esp-idf`, tools in `C:\Espressif`)

Both install ESP-IDF **v5.5.5** for `esp32p4` (same as CI and M5MusicTab5; anything below 5.4.2 has a
Tab5 USB host bug), gitleaks, and set `core.hooksPath .githooks`.

## Building

Activate ESP-IDF in the shell first, then:

```bash
cd firmware
idf.py build
idf.py -p <port> flash monitor   # Tab5 USB-C port; the keyboard is on the USB-A port
```

- macOS activate: `. ~/esp/esp-idf-v5.5.5/export.sh`
- Windows activate: `. C:\Espressif\tools\Microsoft.v5.5.5.PowerShell_profile.ps1` (also a desktop shortcut "ESP-IDF v5.5.5 PowerShell")

CI (`.github/workflows/firmware.yml`) builds every push that touches `firmware/`. Run a local build before pushing firmware changes.

## Layout

- `firmware/main/main.c`: test app (USB MIDI in, logs keys, lights a scale on channel 1)
- `firmware/components/usb_midi_host/`: USB MIDI host, in + out, Tab5 USB-A (based on M5MusicTab5)
- `firmware/components/midi_file/`: SMF parser copied from M5MusicTab5; keep `THIRD_PARTY.md` up to date when copying more
- Board support comes from the `espressif/m5stack_tab5` managed component (`firmware/main/idf_component.yml`)

## Related

- Songs: Pandora, the Telegram bot in `bozek-lu/raspberry-projects` (`pi/home/piano.py`, `pi/home/server/piano/`),
  turns YouTube links into MIDI (right hand channel 1, left hand channel 2) and serves them at
  `http://homepi.local:8095/songs.json` (`[{file, title, url, seconds, notes, added}]`, files at `/<file>`).
- EZ-310 light channels: Function 042/043, default channels 1/2.

## Rules

- Never commit MIDI songs (`songs/` is ignored) or secrets (Wi-Fi credentials go in an ignored local file).
- Hardware behaviour in `docs/hardware.md` comes from manual excerpts until tested; mark what has been verified on the real keyboard.
