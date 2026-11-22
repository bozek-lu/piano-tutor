/*
 * Piano tutor, milestone 1-2 test firmware for the Tab5:
 *  - powers the USB-A port and opens the EZ-310 as a USB MIDI device
 *  - logs every key you press (serial monitor)
 *  - when the keyboard connects, lights a C major scale on channel 1 (EZ-310 Light Part 1)
 */
#include <stdbool.h>

#include "bsp/m5stack_tab5.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb_midi_host.h"

static const char *TAG = "piano-tutor";

/* EZ-310 defaults: Function 042 Light Part 1 = channel 1, 043 Light Part 2 = channel 2 */
#define LIGHT_CH_RIGHT 0
#define LIGHT_CH_LEFT  1

static const char *note_name(uint8_t note)
{
	static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
	return names[note % 12];
}

static void on_midi(const uint8_t *msg, size_t len, void *arg)
{
	uint8_t type = msg[0] & 0xF0;
	if (len == 3 && type == 0x90 && msg[2] > 0) {
		ESP_LOGI(TAG, "key down %s%d (%u) vel %u ch %u", note_name(msg[1]), msg[1] / 12 - 1, msg[1], msg[2],
		         (msg[0] & 0x0F) + 1);
	} else if (len == 3 && (type == 0x80 || type == 0x90)) {
		ESP_LOGD(TAG, "key up %u", msg[1]);
	} else if (len == 3 && type == 0xB0) {
		ESP_LOGI(TAG, "control %u = %u", msg[1], msg[2]);
	}
}

static void light(uint8_t channel, uint8_t note, bool on)
{
	uint8_t msg[3] = {(uint8_t)((on ? 0x90 : 0x80) | channel), note, on ? 64 : 0};
	esp_err_t err = usb_midi_host_send(msg, sizeof msg, 100);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "send failed: %s", esp_err_to_name(err));
	}
}

static void light_scale(void)
{
	static const uint8_t scale[] = {60, 62, 64, 65, 67, 69, 71, 72};
	ESP_LOGI(TAG, "lighting C major scale on channel %d", LIGHT_CH_RIGHT + 1);
	for (size_t i = 0; i < sizeof scale; i++) {
		light(LIGHT_CH_RIGHT, scale[i], true);
		vTaskDelay(pdMS_TO_TICKS(400));
		light(LIGHT_CH_RIGHT, scale[i], false);
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "Piano tutor test firmware");

	/* USB Host Library + 5 V on the USB-A port (powered from USB-C / battery, 500 mA limit) */
	ESP_ERROR_CHECK(bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, true));
	ESP_ERROR_CHECK(usb_midi_host_start(on_midi, NULL));

	bool was_connected = false;
	while (true) {
		bool connected = usb_midi_host_connected();
		if (connected && !was_connected) {
			uint16_t vid, pid;
			usb_midi_host_ids(&vid, &pid);
			ESP_LOGI(TAG, "keyboard connected (VID 0x%04X PID 0x%04X), play some keys", vid, pid);
			if (usb_midi_host_can_send()) {
				vTaskDelay(pdMS_TO_TICKS(1000));
				light_scale();
			} else {
				ESP_LOGW(TAG, "keyboard has no MIDI OUT endpoint, cannot light keys");
			}
		} else if (!connected && was_connected) {
			ESP_LOGI(TAG, "keyboard disconnected");
		}
		was_connected = connected;
		vTaskDelay(pdMS_TO_TICKS(200));
	}
}
