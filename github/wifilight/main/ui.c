/*
 * bleSpeakerUi.c
 *
 *  Created on: 2019��1��6��
 *      Author: LinxSong
 */

#include "ui.h"

#include "cicleled.h"
#include "string.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_system.h"
#define TAG "UITask"
static xQueueHandle *xKey_Queue;
static xQueueHandle *xLed_Queue;
esp_timer_handle_t timerHandle = NULL;
static void timerPeriodicCb(void *arg);
esp_timer_create_args_t timerPeriodicArg = { .callback = &timerPeriodicCb,
    .arg = NULL, .name = "Timer" };

static eKey_t eGetKey(void)
{
	uint8_t valCur;
	static eKey_t eKey;
	static uint8_t valFilt = 0;
	static uint8_t valOld = 0;
	static uint8_t cntFilt = 0;
	static uint8_t oldPressed = 0;
	static uint8_t oldReleased = 0;
	static uint16_t cntPressed = 0;
	static uint8_t flagLongPressed = 0;
	if (!gpio_get_level(KEY_LEFT_IO_BTN))
	{
		valCur = 1;
	}
	else
	{
		valCur = 0;
	}
	if (!gpio_get_level(KEY_RIGHT_IO_BTN))
	{
		valCur |= 2;
	}
	eKey = eKeyNone;
	if (valCur != valFilt)
	{
		cntFilt++;
		if (cntFilt >= 5)
		{
			valFilt = valCur;
			cntFilt = 0;
		}
	}
	else
	{
		cntFilt = 0;
	}
	if (valFilt != valOld)
	{
		if (!valFilt)    //�ɿ�
		{
			if (oldPressed)
			{
				if (!flagLongPressed)
				{
					if (valOld & 0x01)
					{
						eKey = eKeyLeftRelease;
					}
					if (valOld & 0x02)
					{
						eKey = eKeyRightRelease;
					}
					if ((valOld & 0x03) == 0x03)
					{
						eKey = eKeyAllRelease;
					}
				}
				flagLongPressed = 0;
			}
			oldPressed = 0;
		}
		else
		{
			oldPressed = 1;
			if (oldReleased)
			{
				if (valFilt & 0x01)
				{
					eKey = eKeyLeftPress;
				}
				if (valFilt & 0x02)
				{
					eKey = eKeyRightPress;
				}
				if ((valFilt & 0x03) == 0x03)
				{
					eKey = eKeyAllPress;
				}
			}
			oldReleased = 0;
		}
		valOld = valFilt;
	}
	else
	{
		if (valFilt)
		{
			if (cntPressed < 65535)
				cntPressed++;
			if (cntPressed == 200)
			{
				flagLongPressed = 1;
				if (valFilt & 0x01)
				{
					eKey = eKeyLeftLong;
				}
				if (valFilt & 0x02)
				{
					eKey = eKeyRightLong;
				}
				if ((valFilt & 0x03) == 0x03)
				{
					eKey = eKeyAllLong;
				}
			}
		}
		else
		{
			cntPressed = 0;
			oldReleased = 1;
		}
	}
	return eKey;
}
static void vLedProcess(sLed_t *psLed, uint8_t uUpdated)
{
	uint8_t paLedPtr[24];
	static uint16_t usCnt = 0;
	static uint16_t usCntMax = 0;
	static uint8_t ucIdx = 0;
	uint8_t i = 0;
	switch (psLed->eLedMod)
		{
	case eLedModOff:
		usCnt = 0;
		usCntMax = 0;
		ucIdx = 0;
		if (uUpdated)
		{
			memset(paLedPtr, 0, sizeof(paLedPtr));
			cicleled_write(paLedPtr, sizeof(paLedPtr));
		}
		break;
	case eLedModUser:
		usCnt = 0;
		usCntMax = 0;
		ucIdx = 0;
		if (uUpdated)
		{
			for (i = 0; i < 8; i++)
			{
				paLedPtr[i * 3] = (psLed->RGB[i] >> 8) & 0xFF;
				paLedPtr[i * 3 + 1] = (psLed->RGB[i]) & 0xFF;
				paLedPtr[i * 3 + 2] = (psLed->RGB[i] >> 16) & 0xFF;
			}
			cicleled_write(paLedPtr, sizeof(paLedPtr));
		}
		break;
	case eLedModWiFiStp1:
	case eLedModWiFiStp2:
		if (psLed->eLedMod == eLedModWiFiStp1)
		{
			usCntMax = 20;
		}
		else
		{
			usCntMax = 5;
		}
		if (usCnt >= usCntMax)
		{
			uint8_t i;
			usCnt = 0;
			memset(paLedPtr, 0, sizeof(paLedPtr));
			paLedPtr[ucIdx * 3 + 1] = 0xFF / 64;  //ͷ��
			paLedPtr[ucIdx * 3 + 2] = 0xFF / 64;  //ͷ��
			if (ucIdx > 2)
			{
				paLedPtr[(ucIdx - 1) * 3 + 1] = 0xFF / 32;  //����
				paLedPtr[(ucIdx - 1) * 3 + 2] = 0xFF / 32;  //����
				paLedPtr[(ucIdx - 2) * 3 + 1] = 0xFF / 2;  //����
				paLedPtr[(ucIdx - 2) * 3 + 2] = 0xFF / 2;  //����
				paLedPtr[(ucIdx - 3) * 3 + 1] = 0xFF;  //�ĺ�
				paLedPtr[(ucIdx - 3) * 3 + 2] = 0xFF;  //����
			}
			else if (ucIdx == 2)
			{
				paLedPtr[(ucIdx - 1) * 3 + 1] = 0xFF / 32;  //����
				paLedPtr[(ucIdx - 1) * 3 + 2] = 0xFF / 32;  //����
				paLedPtr[(ucIdx - 2) * 3 + 1] = 0xFF / 2;  //����
				paLedPtr[(ucIdx - 2) * 3 + 2] = 0xFF / 2;  //����
				paLedPtr[7 * 3 + 1] = 0xFF;  //�ĺ�
				paLedPtr[7 * 3 + 2] = 0xFF;  //����
			}
			else if (ucIdx == 1)
			{
				paLedPtr[(ucIdx - 1) * 3 + 1] = 0xFF / 32;  //����
				paLedPtr[(ucIdx - 1) * 3 + 2] = 0xFF / 32;  //����
				paLedPtr[7 * 3 + 1] = 0xFF / 2;  //����
				paLedPtr[7 * 3 + 2] = 0xFF / 2;  //����
				paLedPtr[6 * 3 + 1] = 0xFF;  //�ĺ�
				paLedPtr[6 * 3 + 2] = 0xFF;  //����
			}
			else if (ucIdx == 0)
			{
				paLedPtr[7 * 3 + 1] = 0xFF / 32;  //����
				paLedPtr[7 * 3 + 2] = 0xFF / 32;  //����
				paLedPtr[6 * 3 + 1] = 0xFF / 2;  //����
				paLedPtr[6 * 3 + 2] = 0xFF / 2;  //����
				paLedPtr[5 * 3 + 1] = 0xFF;  //�ĺ�
				paLedPtr[5 * 3 + 2] = 0xFF;  //����
			}
			cicleled_write(paLedPtr, sizeof(paLedPtr));
			if (ucIdx > 0)
			{
				ucIdx--;
			}
			else
			{
				ucIdx = 7;
			}
		}
		break;
	case eLedModWiFiCntOK:
		break;
	default:
		usCnt = 0;
		usCntMax = 0;
		ucIdx = 0;
		break;
		}
	usCnt++;
}
void vUI_Task(void *parm)
{
	TickType_t xkeyTick;
	eKey_t eKey;
	sLed_t sLed;
	uint8_t updated = 0;
	xkeyTick = xTaskGetTickCount();
	for (;;)
	{
		eKey = eGetKey();
		if (eKey != eKeyNone)
		{
			xQueueSend(*xKey_Queue, &eKey, NULL);
		}
		if (xQueueReceive(*xLed_Queue, &sLed, NULL))
		{
			updated = 1;
		}
		vLedProcess(&sLed, updated);
		updated = 0;
		vTaskDelayUntil(&xkeyTick, (10 / portTICK_RATE_MS));
	}
}
void vUI_SetKeyQueue(xQueueHandle *queue)
{
	xKey_Queue = queue;
}
void vUI_SetLedQueue(xQueueHandle *queue)
{
	xLed_Queue = queue;
}
eKey_t const eKeys[][4] = { { eKeyUpDec, eKeyUpInc, eKeyUpInc, eKeyUpDec }, {
    eKeyRightInc, eKeyRightDec, eKeyRightDec, eKeyRightInc }, { eKeyLeftInc,
    eKeyLeftDec, eKeyLeftDec, eKeyLeftInc }, };
gpio_num_t const KeyIOTable[] = { KEY_LEFT_IO_B, KEY_LEFT_IO_A, KEY_RIGHT_IO_B,
KEY_RIGHT_IO_A, KEY_UP_IO_B,
KEY_UP_IO_A };
static void timerPeriodicCb(void *arg)
{
	eKey_t eKey = eKeyNone; // @suppress("Symbol is not resolved")
	static uint8_t cntFilter = 0;
	static uint8_t keyOld = 0;
	static uint8_t init = 0;
	uint8_t keyCur = 0;
	uint8_t i;
	keyCur = 0;
	for (i = 0; i < 6; i++)
	{
		keyCur <<= 1;
		keyCur |= gpio_get_level(KeyIOTable[i]);
	}
	if (keyCur != keyOld)
	{
		cntFilter++;
		if (cntFilter > 5)
		{
			for (i = 0; i < 3; i++)
			{
				if ((keyOld & (0x01 << (i * 2))) != (keyCur & (0x01 << (i * 2))))
				{
					eKey = eKeys[i][(keyOld >> (i * 2)) & 0x03];
					if (init)
					{
						xQueueSendFromISR(*xKey_Queue, &eKey, NULL);
					}
					init = 1;
				}
			}
			keyOld = keyCur;
		}
	}
	else
	{
		cntFilter = 0;
	}
}
#define GPIO_INPUT_PIN_SEL  ((1ULL<<KEY_LEFT_IO_A) | (1ULL<<KEY_RIGHT_IO_A)|(1ULL << KEY_UP_IO_A))
void vUI_Init(void)
{
	cicleled_init();
	gpio_set_direction(KEY_LEFT_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_LEFT_IO_B, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_LEFT_IO_BTN, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_RIGHT_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_RIGHT_IO_B, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_RIGHT_IO_BTN, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_UP_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_UP_IO_B, GPIO_MODE_INPUT);
	ESP_ERROR_CHECK(esp_timer_create(&timerPeriodicArg, &timerHandle));
	ESP_ERROR_CHECK(esp_timer_start_periodic(timerHandle, 100));

}
