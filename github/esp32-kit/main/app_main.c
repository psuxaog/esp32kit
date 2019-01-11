#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "kitui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "oled.h"
#include "esp_smartconfig.h"
#include "cJSON.h"
#include "cicleled.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "oled.h"
static const char *TAG = "MQTT_KIT";
static void smartconfig_example_task(void *param);
static EventGroupHandle_t wifi_event_group;
uint8_t phone_ip[4] = { 0 };
const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;
const int CONNECTED_BIT = BIT2;
const int ESPTOUCH_DONE_BIT = BIT3;
const int ESPTOUCH_START = BIT4;
typedef enum
{
	SmartCfgNone,
	SmartCfgStp1,
	SmartCfgStp2,
	SmartCfgOver,
}SmartCfg_t;
static SmartCfg_t SmartCfg=SmartCfgNone;
xQueueHandle xKeyQueue;

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
			ESP_LOGI(TAG, "phone ip : %d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
			    phone_ip[2], phone_ip[3]);
		}
		SmartCfg = SmartCfgOver;
		OLED_Print("SMART OVER");
		xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
		break;
	default:
		break;
		}
}
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;
	// your_context_t *context = event->context;
	switch (event->event_id)
		{
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		OLED_Print("MQTT_EVENT_CONNECTED");
		msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_subscribe(client, "ledrgb", 1);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

		msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
		ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;

	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		OLED_Print("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		printf("DATA=%.*s\r\n", event->data_len, event->data);
		OLED_Print("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		OLED_Print("DATA=%.*s\r\n", event->data_len, event->data);
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		break;
		}
	return ESP_OK;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
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
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STA_GOT_IP6:
		xEventGroupSetBits(wifi_event_group, IPV6_GOTIP_BIT);
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6");
		char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
		ESP_LOGI(TAG, "IPv6:%s", ip6);
		break;
	default:
		break;
		}
	return ESP_OK;
}
static void wifi_init(void)
{
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	wifi_config_t wifi_config;
	ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config));
	wifi_config.sta.scan_method = WIFI_FAST_SCAN;
	ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_set_auto_connect(true));
	ESP_LOGI(TAG, "start the WIFI SSID:[%s]", CONFIG_WIFI_SSID);
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "Waiting for wifi");
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,
	portMAX_DELAY);
}

static void ui_process_task(void *parm)
{
	eKey_t eKey = eKeyNone;
	for (;;)
	{
		if (xQueueReceive(xKeyQueue, &eKey, NULL))
		{
			switch (eKey)
				{
			case eKeyAllLong:
				ESP_LOGI(TAG, "eKeyAllLong");
				if (SmartCfg == SmartCfgNone)
				{
					ESP_ERROR_CHECK(esp_wifi_disconnect());
					vTaskDelay(10 / portTICK_PERIOD_MS);
					xTaskCreate(smartconfig_example_task, "smartconfig_example_task",
					    4096,
					    NULL, 3, NULL);
					OLED_Print("SMART CONFIG START...");
					SmartCfg = SmartCfgStp1;
				}
				break;
			case eKeyWheelInc:
				ESP_LOGI(TAG,"Wheel INC +++");
				break;
			case eKeyWheelDec:
				ESP_LOGI(TAG,"Wheel DEC");
				break;
			default:
				break;
				}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}
void smartconfig_example_task(void *parm)
{
	EventBits_t uxBits;
	ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
	ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
	while (1)
	{
		uxBits = xEventGroupWaitBits(wifi_event_group,
		    CONNECTED_BIT | ESPTOUCH_DONE_BIT, true,
		    false,
		    portMAX_DELAY);
		if (uxBits & CONNECTED_BIT)
		{
			ESP_LOGI(TAG, "WiFi Connected to ap");
			OLED_Print("WiFi Connected to ap");
		}
		if (uxBits & ESPTOUCH_DONE_BIT)
		{
			ESP_LOGI(TAG, "smartconfig over");
			OLED_Print("SMART CONFIG OVER");
			esp_smartconfig_stop();
			vTaskDelete(NULL);
		}
	}
}
static void mqtt_app_start(void)
{
	esp_mqtt_client_config_t mqtt_cfg = { .host = "39.108.162.165", .port = 1883,
	    .lwt_topic = "windows", .event_handle = mqtt_event_handler, .username =
	        "admin", .password = "public",

// .user_context = (void *)your_context
	    };

	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_start(client);
}

void app_main()
{
	ESP_LOGI(TAG, "[APP] Startup..");
	ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
	OLED_Init();
	OLED_Print("PROMRAM START...");
	xKeyQueue = xQueueCreate(10, sizeof(eKey_t));
	vUI_SetKeyQueue(&xKeyQueue);
	vUI_Init();
	xTaskCreate(ui_process_task, "ui_process_task", 4096, NULL, 4, NULL);
	xTaskCreate(vUI_Task, "ui_task", 4096, NULL, 5, NULL);
	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
	esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

	nvs_flash_init();
	wifi_init();
	mqtt_app_start();
}
