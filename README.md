# piano-tutor

A Flowkey-style practice helper for a **Yamaha EZ-310** (lighted keys), running on an
**M5Stack Tab5**. Load a MIDI song, the Tab5 lights the next notes on the keyboard,
waits for the right keys and shows progress on its 5" screen.

| Path | What |
| --- | --- |
| `firmware/` | ESP-IDF app for the Tab5 (ESP32-P4). Skeleton for now |
| `docs/hardware.md` | Parts, wiring, EZ-310 settings, known risks |

## Hardware

- M5Stack Tab5 Kit K145 (ESP32-P4, 5" 1280x720 touch, NP-F550 battery, microSD), from Reichelt
- USB-A to USB-B cable: Tab5 **USB-A host port** -> EZ-310 USB TO HOST.
  The Tab5 USB-C port is only for charging and flashing; it cannot host the keyboard.
- USB-C charger, microSD card for songs

## Plan

1. **USB bring-up:** Tab5 enumerates the EZ-310 (USB audio + MIDI composite) and logs key presses.
   Riskiest step, done first.
2. **Lights:** send notes on the EZ-310 light channels (Function 042/043, default ch 1/2) and confirm keys light.
3. **Wait mode:** load a MIDI file from microSD, light the next note(s), advance on the correct key.
4. **Screen:** song list, piano roll, tempo, loop a section, left/right hand, score.
5. **Extras:** upload songs over Wi-Fi (separate ESP32-C6, so it can run alongside USB host).

## Workflow

```bash
git commit -am "..." && git push
```

GitHub Actions builds the firmware for `esp32p4` on every push that touches `firmware/`, so
compile errors show up even without a local toolchain. To flash, install ESP-IDF >= 5.4.2
(see `firmware/README.md`). Older IDF versions and M5Stack's Arduino fork have a Tab5
full-speed USB host bug (m5stack/M5Stack#344).

## Secrets

Nothing secret is committed (future Wi-Fi credentials stay in ignored local files).
Same guards as raspberry-projects: `.gitignore`, a gitleaks pre-commit hook
(`git config core.hooksPath .githooks`, needs gitleaks installed) with rules in
`.gitleaks.toml`, and a gitleaks GitHub Action.

MIDI songs are not committed either (`songs/` is ignored): many are copyrighted.
