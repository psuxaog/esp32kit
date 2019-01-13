/*
 * include std lib
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/*
 * include esplib
 */
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_smartconfig.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
/*
 * include freertos
 */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "oled.h"
#define TAG "FirstProject"

void app_main()
{
	OLED_Init();
	ESP_LOGI(TAG, "Startup...");
	ESP_LOGI(TAG, "Free memory:%d bytes",esp_get_free_heap_size());
	ESP_LOGI(TAG, "Target:%s",CONFIG_BOARD_TARGET);
	OLED_Print("Startup...");
	OLED_Print("Free memory:%d bytes",esp_get_free_heap_size());
	OLED_Print("Target:%s",CONFIG_BOARD_TARGET);

}
