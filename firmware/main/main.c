#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "piano-tutor";

void app_main(void)
{
    ESP_LOGI(TAG, "Piano tutor skeleton running");

    // Milestone 1: start USB host on the Tab5 USB-A port, enumerate the EZ-310
    // and log incoming MIDI note events.

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
