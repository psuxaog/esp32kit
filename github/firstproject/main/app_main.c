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
#include "kitui.h"
#include "smartcfg.h"
/*
 * Global Val Def
 */
#define TAG "FirstProject"
static xQueueHandle xKeyQueue = NULL;
static EventGroupHandle_t EventGroupWiFi;

static const int BIT_CONNECT_OK = BIT0;
static const int BIT_IPV4_GOT_IP = BIT1;
static const int BIT_IPV6_GOT_IP = BIT2;

/*
 *
 */
static esp_err_t WiFiEventHandle(void *ctx, system_event_t *event)
{
	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(EventGroupWiFi, BIT_CONNECT_OK);
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(EventGroupWiFi, BIT_CONNECT_OK);
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
		break;
	case SYSTEM_EVENT_AP_STA_GOT_IP6:
		xEventGroupSetBits(EventGroupWiFi, BIT_IPV6_GOT_IP);
		ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STA_GOT_IP6");
		char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
		ESP_LOGI(TAG, "IPv6:%s", ip6);
		break;
	default:
		break;
	}
	return ESP_OK;
}
static void AppUi_Task(void *parm)
{
	eKey_t eKey = eKeyNone;
	for (;;)
	{
		if (xQueueReceive(xKeyQueue, &eKey, NULL))
		{
			switch (eKey)
			{
			case eKeyWheelDec:
				break;
			case eKeyWheelInc:
				break;
			case eKeyWheelLong:
				break;
			case eKeyWheelPress:
				break;
			case eKeyWheelRelease:
				break;
			case eKeyLong:
				SC_StartSmart();
				break;
			case eKeyPress:
				break;
			case eKeyRelease:
				break;
			case eKeyAllLong:
				break;
			case eKeyAllPress:
				break;
			case eKeyAllRelease:
				break;
			default:
				break;
			}
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
	vTaskDelete(NULL);
}
static void wifi_init(void)
{
	tcpip_adapter_init();
	EventGroupWiFi = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(WiFiEventHandle, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	wifi_config_t wifi_cfg;
	ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg));
	wifi_cfg.sta.scan_method = WIFI_FAST_SCAN;
	ESP_LOGI(TAG, "WIFI SSID:%s", wifi_cfg.sta.ssid);
	ESP_LOGI(TAG, "WIFI PASSWORD:%s", wifi_cfg.sta.password);
	OLED_Print("WIFI SSID:%s", wifi_cfg.sta.ssid);
	OLED_Print("WIFI PASSWORD:%s", wifi_cfg.sta.password);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
	ESP_ERROR_CHECK(esp_wifi_set_auto_connect(true));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "Watting for wifi...");
	xEventGroupWaitBits(EventGroupWiFi, BIT_CONNECT_OK, false, true,
	portMAX_DELAY);
	ESP_LOGI(TAG, "Connect OK");
}
void app_main()
{
	nvs_flash_init();
	OLED_Init();
	ESP_LOGI(TAG, "Startup...");
	ESP_LOGI(TAG, "Free memory:%d bytes", esp_get_free_heap_size());
	ESP_LOGI(TAG, "Target:%s", CONFIG_BOARD_TARGET);
	OLED_Print("Startup...");
	OLED_Print("Free memory:%d bytes", esp_get_free_heap_size());
	OLED_Print("Target:%s", CONFIG_BOARD_TARGET);
	xKeyQueue = xQueueCreate(2, sizeof(eKey_t));
	vUI_Init(&xKeyQueue);
	xTaskCreate(vUI_Task, "UI_Task", 1280, NULL, 5, NULL);
	xTaskCreate(AppUi_Task, "AppUI_Task", 4096, NULL, 5, NULL);
	wifi_init();

}
