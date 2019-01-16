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
#include "cJSON.h"
/*
 * Global Val Def
 */
#define TAG "FirstProject"
static xQueueHandle xKeyQueue = NULL;
static EventGroupHandle_t EventGroupWiFi;

static const int BIT_CONNECT_OK = BIT0;
static const int BIT_IPV4_GOT_IP = BIT1;
static const int BIT_IPV6_GOT_IP = BIT2;
typedef struct
{
	struct
	{
		char *ip;
		uint8_t mac[6];
		char *ssid;
	} Net;
	struct
	{
		char *pn;
		uint16_t id;
		uint8_t type;
	} Device;
} DeviceInfo_t;
DeviceInfo_t DeviceInfo;

static void PushDevInfo(DeviceInfo_t *device);
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
		DeviceInfo.Net.ip = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
		ESP_ERROR_CHECK(esp_read_mac(DeviceInfo.Net.mac,ESP_MAC_WIFI_STA))
		;
		PushDevInfo(&DeviceInfo);
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
				ESP_LOGI(TAG, "eKeyWheelDec");
				OLED_Print("Dec");
				break;
			case eKeyWheelInc:
				ESP_LOGI(TAG, "eKeyWheelInc+++");
				OLED_Print("Inc+++");
				break;
			case eKeyWheelLong:
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
	DeviceInfo.Net.ssid = (char *)&wifi_cfg.sta.ssid[0];
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
static void PushDevInfo(DeviceInfo_t *device)
{
	cJSON *root = NULL;
	cJSON *objDev = NULL;
	cJSON *objNet = NULL;
	char *macstr = pvPortMalloc(18);
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "netcfg", objNet = cJSON_CreateObject());
	sprintf(macstr, "%2X:%2X:%2X:%2X:%2X:%2X", device->Net.mac[0],
			device->Net.mac[1], device->Net.mac[2], device->Net.mac[3],
			device->Net.mac[4], device->Net.mac[5]);
	cJSON_AddStringToObject(objNet, "mac", macstr);
	vPortFree(macstr);
	if (device->Net.ip != NULL)
	{
		cJSON_AddStringToObject(objNet, "ip", device->Net.ip);
	}
	if (device->Net.ssid != NULL)
	{
		cJSON_AddStringToObject(objNet, "ssid", device->Net.ssid);
	}

	cJSON_AddItemToObject(root, "device", objDev = cJSON_CreateObject());

	if (device->Device.pn != NULL)
	{
		cJSON_AddStringToObject(objDev, "pn", device->Device.pn);
	}
	cJSON_AddNumberToObject(objDev, "id", device->Device.id);
	cJSON_AddNumberToObject(objDev, "type", device->Device.type);
	/* Print to text */
	printf("cJson Begin:\r\n%s\r\nEnd", cJSON_Print(root));
	cJSON_Delete(root);
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
	xTaskCreate(vUI_Task, "UI_Task", 4096, NULL, 5, NULL);
	xTaskCreate(AppUi_Task, "AppUI_Task", 4096, NULL, 4, NULL);
	wifi_init();
}
