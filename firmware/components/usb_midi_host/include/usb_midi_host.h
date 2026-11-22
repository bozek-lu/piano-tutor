/**
 * USB MIDI host for the Tab5 USB-A port: receive key presses from the keyboard and
 * send notes back to it (on the EZ-310 light channels this lights the keys).
 *
 * Based on service_usb_host.c from M5MusicTab5 (MIT, Copyright (c) 2026 Fangbrbr),
 * reduced to MIDI only and extended with a MIDI OUT endpoint.
 *
 * The caller must already have the USB Host Library installed and running
 * (on the Tab5: bsp_usb_host_start(), which also switches on 5 V for the USB-A port).
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Called from the USB client task for every incoming MIDI message (1-3 bytes). */
typedef void (*usb_midi_rx_cb_t)(const uint8_t *msg, size_t len, void *arg);

/** Register as a USB host client and start handling MIDI devices. */
esp_err_t usb_midi_host_start(usb_midi_rx_cb_t rx_cb, void *arg);

/** A MIDI device is open and its endpoints are ready. */
bool usb_midi_host_connected(void);

/** The connected device has a MIDI OUT endpoint (needed to light keys). */
bool usb_midi_host_can_send(void);

/** Send one channel message (e.g. {0x90, 60, 64}). Waits up to timeout_ms for the previous one. */
esp_err_t usb_midi_host_send(const uint8_t *msg, size_t len, uint32_t timeout_ms);

/** Vendor / product ID of the connected device (0 when none). */
void usb_midi_host_ids(uint16_t *vid, uint16_t *pid);

#ifdef __cplusplus
}
#endif
