/*
 * USB MIDI host: one class-compliant USB MIDI device (e.g. Yamaha EZ-310) on the Tab5 USB-A port.
 *
 * Based on service_usb_host.c from M5MusicTab5 (MIT, Copyright (c) 2026 Fangbrbr):
 * descriptor scan for the MIDI Streaming interface and USB-MIDI packet decoding.
 * Changes: MIDI only (no HID), MIDI OUT endpoint + send API, callback instead of a MIDI bus,
 * own client task, clean shutdown of in-flight transfers when the device is unplugged.
 */
#include "usb_midi_host.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "usb/usb_helpers.h"
#include "usb/usb_host.h"

static const char *TAG = "usb_midi";

#define MIDI_SUBCLASS_STREAMING 0x03
#define IN_BUF_SIZE             64
#define OUT_BUF_SIZE            64

static usb_host_client_handle_t s_client;
static usb_device_handle_t s_dev;
static usb_transfer_t *s_in, *s_out;
static uint8_t s_intf = 0xFF;
static uint8_t s_ep_in, s_ep_out;
static volatile bool s_ready, s_closing, s_in_busy, s_out_busy;
static SemaphoreHandle_t s_out_free;
static uint16_t s_vid, s_pid;
static uint8_t s_pending_addr;

static usb_midi_rx_cb_t s_rx_cb;
static void *s_rx_arg;

/* USB-MIDI event packet -> MIDI bytes. Byte 0 is [cable:4][CIN:4]. */
static void decode_packet(const uint8_t pkt[4])
{
	static const uint8_t len_by_cin[16] = {0, 0, 2, 3, 3, 1, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1};
	uint8_t len = len_by_cin[pkt[0] & 0x0F];
	if (len && s_rx_cb) {
		s_rx_cb(&pkt[1], len, s_rx_arg);
	}
}

static void in_done(usb_transfer_t *xfer)
{
	if (xfer->status == USB_TRANSFER_STATUS_COMPLETED) {
		for (int i = 0; i + 3 < xfer->actual_num_bytes; i += 4) {
			decode_packet(&xfer->data_buffer[i]);
		}
		if (!s_closing && usb_host_transfer_submit(xfer) == ESP_OK) {
			return;
		}
	}
	s_in_busy = false;
}

static void out_done(usb_transfer_t *xfer)
{
	if (xfer->status != USB_TRANSFER_STATUS_COMPLETED) {
		ESP_LOGW(TAG, "OUT transfer status %d", xfer->status);
	}
	s_out_busy = false;
	xSemaphoreGive(s_out_free);
}

static void device_open(uint8_t addr)
{
	if (s_dev) {
		ESP_LOGW(TAG, "second USB device ignored");
		return;
	}
	if (usb_host_device_open(s_client, addr, &s_dev) != ESP_OK) {
		ESP_LOGW(TAG, "device open failed");
		s_dev = NULL;
		return;
	}
	const usb_device_desc_t *dev_desc;
	if (usb_host_get_device_descriptor(s_dev, &dev_desc) == ESP_OK) {
		s_vid = dev_desc->idVendor;
		s_pid = dev_desc->idProduct;
	}
	ESP_LOGI(TAG, "device VID=0x%04X PID=0x%04X", s_vid, s_pid);

	const usb_config_desc_t *cfg;
	if (usb_host_get_active_config_descriptor(s_dev, &cfg) != ESP_OK) {
		ESP_LOGW(TAG, "no config descriptor");
		goto fail;
	}

	/* Find the MIDI Streaming interface (audio class, subclass 3) and its bulk/interrupt endpoints.
	 * Composite devices like the EZ-310 also have audio interfaces with isochronous endpoints. */
	int offset = 0;
	uint16_t in_mps = IN_BUF_SIZE;
	bool in_midi = false;
	const usb_standard_desc_t *d = (const usb_standard_desc_t *)cfg;
	while ((d = usb_parse_next_descriptor(d, cfg->wTotalLength, &offset)) != NULL) {
		if (d->bDescriptorType == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
			const usb_intf_desc_t *intf = (const usb_intf_desc_t *)d;
			if (s_intf != 0xFF && intf->bInterfaceNumber != s_intf) {
				break; /* past the MIDI interface */
			}
			in_midi = intf->bInterfaceClass == USB_CLASS_AUDIO &&
			          intf->bInterfaceSubClass == MIDI_SUBCLASS_STREAMING;
			if (in_midi) {
				s_intf = intf->bInterfaceNumber;
			}
		} else if (in_midi && d->bDescriptorType == USB_B_DESCRIPTOR_TYPE_ENDPOINT) {
			const usb_ep_desc_t *ep = (const usb_ep_desc_t *)d;
			int type = USB_EP_DESC_GET_XFERTYPE(ep);
			if (type != USB_TRANSFER_TYPE_BULK && type != USB_TRANSFER_TYPE_INTR) {
				continue;
			}
			if (USB_EP_DESC_GET_EP_DIR(ep)) {
				if (!s_ep_in) {
					s_ep_in = ep->bEndpointAddress;
					in_mps = USB_EP_DESC_GET_MPS(ep);
				}
			} else if (!s_ep_out) {
				s_ep_out = ep->bEndpointAddress;
			}
		}
	}
	if (s_intf == 0xFF || !s_ep_in) {
		ESP_LOGW(TAG, "not a USB MIDI device (no MIDI Streaming IN endpoint)");
		goto fail;
	}
	if (usb_host_interface_claim(s_client, s_dev, s_intf, 0) != ESP_OK) {
		ESP_LOGW(TAG, "interface %u claim failed", s_intf);
		s_intf = 0xFF;
		goto fail;
	}

	/* IN transfers must be a multiple of the endpoint's max packet size */
	if (usb_host_transfer_alloc(usb_round_up_to_mps(IN_BUF_SIZE, in_mps), 0, &s_in) != ESP_OK ||
	    (s_ep_out && usb_host_transfer_alloc(OUT_BUF_SIZE, 0, &s_out) != ESP_OK)) {
		ESP_LOGW(TAG, "transfer alloc failed");
		goto fail;
	}
	s_in->device_handle = s_dev;
	s_in->bEndpointAddress = s_ep_in;
	s_in->callback = in_done;
	s_in->num_bytes = usb_round_up_to_mps(IN_BUF_SIZE, in_mps);
	if (s_out) {
		s_out->device_handle = s_dev;
		s_out->bEndpointAddress = s_ep_out;
		s_out->callback = out_done;
	}
	s_closing = false;
	s_in_busy = true;
	if (usb_host_transfer_submit(s_in) != ESP_OK) {
		s_in_busy = false;
		ESP_LOGW(TAG, "IN submit failed");
		goto fail;
	}
	xSemaphoreGive(s_out_free);
	s_ready = true;
	ESP_LOGI(TAG, "MIDI ready: interface %u, IN 0x%02X, OUT %s0x%02X", s_intf, s_ep_in,
	         s_ep_out ? "" : "none ", s_ep_out);
	return;

fail:
	s_closing = true;
}

/* Called from the client task loop: finish closing once no transfer is in flight. */
static void device_close_step(void)
{
	if (!s_dev || !s_closing) {
		return;
	}
	s_ready = false;
	if (s_in_busy || s_out_busy) {
		/* Unplugged: make the stack return the pending transfers, then try again next loop */
		if (s_in_busy && s_ep_in) {
			usb_host_endpoint_halt(s_dev, s_ep_in);
			usb_host_endpoint_flush(s_dev, s_ep_in);
		}
		if (s_out_busy && s_ep_out) {
			usb_host_endpoint_halt(s_dev, s_ep_out);
			usb_host_endpoint_flush(s_dev, s_ep_out);
		}
		return;
	}
	xSemaphoreTake(s_out_free, 0);
	if (s_intf != 0xFF) {
		usb_host_interface_release(s_client, s_dev, s_intf);
	}
	if (s_in) {
		usb_host_transfer_free(s_in);
	}
	if (s_out) {
		usb_host_transfer_free(s_out);
	}
	usb_host_device_close(s_client, s_dev);
	s_dev = NULL;
	s_in = s_out = NULL;
	s_intf = 0xFF;
	s_ep_in = s_ep_out = 0;
	s_vid = s_pid = 0;
	ESP_LOGI(TAG, "device closed");
}

static void client_event(const usb_host_client_event_msg_t *msg, void *arg)
{
	switch (msg->event) {
	case USB_HOST_CLIENT_EVENT_NEW_DEV:
		s_pending_addr = msg->new_dev.address;
		break;
	case USB_HOST_CLIENT_EVENT_DEV_GONE:
		ESP_LOGI(TAG, "device unplugged");
		s_closing = true;
		break;
	default:
		break;
	}
}

static void client_task(void *arg)
{
	while (true) {
		usb_host_client_handle_events(s_client, pdMS_TO_TICKS(50));
		if (s_pending_addr) {
			uint8_t addr = s_pending_addr;
			s_pending_addr = 0;
			device_open(addr);
		}
		device_close_step();
	}
}

esp_err_t usb_midi_host_start(usb_midi_rx_cb_t rx_cb, void *arg)
{
	if (s_client) {
		return ESP_OK;
	}
	s_rx_cb = rx_cb;
	s_rx_arg = arg;
	s_out_free = xSemaphoreCreateBinary();
	if (!s_out_free) {
		return ESP_ERR_NO_MEM;
	}
	const usb_host_client_config_t cfg = {
		.is_synchronous = false,
		.max_num_event_msg = 5,
		.async = {.client_event_callback = client_event, .callback_arg = NULL},
	};
	esp_err_t err = usb_host_client_register(&cfg, &s_client);
	if (err != ESP_OK) {
		return err;
	}
	if (xTaskCreate(client_task, "usb_midi", 4096, NULL, 5, NULL) != pdPASS) {
		return ESP_ERR_NO_MEM;
	}
	return ESP_OK;
}

bool usb_midi_host_connected(void)
{
	return s_ready;
}

bool usb_midi_host_can_send(void)
{
	return s_ready && s_out;
}

esp_err_t usb_midi_host_send(const uint8_t *msg, size_t len, uint32_t timeout_ms)
{
	if (!usb_midi_host_can_send() || len == 0 || len > 3 || msg[0] < 0x80) {
		return ESP_ERR_INVALID_STATE;
	}
	if (xSemaphoreTake(s_out_free, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
		return ESP_ERR_TIMEOUT;
	}
	if (!s_ready) {
		xSemaphoreGive(s_out_free);
		return ESP_ERR_INVALID_STATE;
	}
	/* Cable 0; for channel messages the CIN is the status high nibble */
	uint8_t *p = s_out->data_buffer;
	memset(p, 0, 4);
	p[0] = msg[0] >= 0xF0 ? 0x0F : (msg[0] >> 4);
	memcpy(&p[1], msg, len);
	s_out->num_bytes = 4;
	s_out_busy = true;
	esp_err_t err = usb_host_transfer_submit(s_out);
	if (err != ESP_OK) {
		s_out_busy = false;
		xSemaphoreGive(s_out_free);
	}
	return err;
}

void usb_midi_host_ids(uint16_t *vid, uint16_t *pid)
{
	if (vid) {
		*vid = s_vid;
	}
	if (pid) {
		*pid = s_pid;
	}
}
