/*
 * smartcfg.c
 *
 *  Created on: 2019年1月13日
 *      Author: linx
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
#define TAG "SmartConfig"
static EventGroupHandle_t EventGroupSmartConfig = NULL;
static const int BIT_ESPTOUCH_DONE = BIT0;
static const int BIT_ESPTOUCH_START = BIT1;
static const int BIT_CONNECT = BIT2;
static SmartCfg_t SmartCfg = SmartCfgNone;
static void sc_callback(smartconfig_status_t status, void *pdata)
{
	switch (status)
	{
	case SC_STATUS_WAIT:
		ESP_LOGI(TAG, "SC_STATUS_WAIT");
		break;
	case SC_STATUS_FIND_CHANNEL:
		ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
		break;
	case SC_STATUS_GETTING_SSID_PSWD:
		ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
		SmartCfg = SmartCfgStp2;
		break;
	case SC_STATUS_LINK:
		ESP_LOGI(TAG, "SC_STATUS_LINK");
		wifi_config_t *wifi_config = pdata;
		ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
		ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
		OLED_Print("SSID:%s", wifi_config->sta.ssid);
		OLED_Print("PASSWORD:%s", wifi_config->sta.password);
		ESP_ERROR_CHECK(esp_wifi_disconnect())
		;
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config))
		;
		ESP_ERROR_CHECK(esp_wifi_connect())
		;
		break;
	case SC_STATUS_LINK_OVER:
		ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
		if (pdata != NULL)
		{
			uint8_t phone_ip[4] =
			{ 0 };
			memcpy(phone_ip, (uint8_t*) pdata, 4);
			ESP_LOGI(TAG, "Phone IP:%d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
					phone_ip[2], phone_ip[3]);
			OLED_Print("Phone IP:%d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
					phone_ip[2], phone_ip[3]);
		}
		SmartCfg = SmartCfgOver;
		OLED_Print("SMART OVER");
		xEventGroupSetBits(EventGroupSmartConfig, BIT_ESPTOUCH_DONE);
		break;
	default:
		break;
	}
}
static void TaskSmartConfig(void *parm)
{
	EventBits_t uxBits;
	ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
	ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
	for (;;)
	{
		uxBits = xEventGroupWaitBits(EventGroupSmartConfig, BIT_ESPTOUCH_DONE,
				true, false, portMAX_DELAY);
		if (uxBits & BIT_ESPTOUCH_DONE)
		{
			ESP_LOGI(TAG, "SmartConfig Over");
			OLED_Print("SMART CONFIG OVER");
			esp_smartconfig_stop();
			vEventGroupDelete(EventGroupSmartConfig);
			vTaskDelay(NULL);
		}
	}
}
void SC_StartSmart(void)
{
	if (SmartCfg == SmartCfgNone)
	{
		EventGroupSmartConfig = xEventGroupCreate();
		ESP_ERROR_CHECK(esp_wifi_disconnect());
		vTaskDelay(10 / portTICK_PERIOD_MS);
		xTaskCreate(TaskSmartConfig, "TaskSmartConfig", 4096,
		NULL, 5, NULL);
		OLED_Print("SMART CONFIG START...");
		SmartCfg = SmartCfgStp1;
	}
}
SmartCfg_t SC_GetSmartCfgState(void)
{
	return SmartCfg;
}
