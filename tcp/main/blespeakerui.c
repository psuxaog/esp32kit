/*
 * bleSpeakerUi.c
 *
 *  Created on: 2019年1月6日
 *      Author: LinxSong
 */

#include "blespeakerui.h"
#include "cicleled.h"
#include "string.h"
#define TAG "UITask"
static xQueueHandle *xKey_Queue;
static xQueueHandle *xLed_Queue;

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
		if (!valFilt)    //松开
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
				paLedPtr[ucIdx * 3 + 1] = 0xFF / 64;  //头红
				paLedPtr[ucIdx * 3 + 2] = 0xFF / 64;  //头蓝
				if (ucIdx > 2)
				{
					paLedPtr[(ucIdx - 1) * 3 + 1] = 0xFF / 32;  //二红
					paLedPtr[(ucIdx - 1) * 3 + 2] = 0xFF / 32;  //二蓝
					paLedPtr[(ucIdx - 2) * 3 + 1] = 0xFF / 2;  //三红
					paLedPtr[(ucIdx - 2) * 3 + 2] = 0xFF / 2;  //三蓝
					paLedPtr[(ucIdx - 3) * 3 + 1] = 0xFF;  //四红
					paLedPtr[(ucIdx - 3) * 3 + 2] = 0xFF;  //四蓝
				}
				else if (ucIdx == 2)
				{
					paLedPtr[(ucIdx - 1) * 3 + 1] = 0xFF / 32;  //二红
					paLedPtr[(ucIdx - 1) * 3 + 2] = 0xFF / 32;  //二蓝
					paLedPtr[(ucIdx - 2) * 3 + 1] = 0xFF / 2;  //三红
					paLedPtr[(ucIdx - 2) * 3 + 2] = 0xFF / 2;  //三蓝
					paLedPtr[7 * 3 + 1] = 0xFF;  //四红
					paLedPtr[7 * 3 + 2] = 0xFF;  //四蓝
				}
				else if (ucIdx == 1)
				{
					paLedPtr[(ucIdx - 1) * 3 + 1] = 0xFF / 32;  //二红
					paLedPtr[(ucIdx - 1) * 3 + 2] = 0xFF / 32;  //二蓝
					paLedPtr[7 * 3 + 1] = 0xFF / 2;  //三红
					paLedPtr[7 * 3 + 2] = 0xFF / 2;  //三蓝
					paLedPtr[6 * 3 + 1] = 0xFF;  //四红
					paLedPtr[6 * 3 + 2] = 0xFF;  //四蓝
				}
				else if (ucIdx == 0)
				{
					paLedPtr[7 * 3 + 1] = 0xFF / 32;  //二红
					paLedPtr[7 * 3 + 2] = 0xFF / 32;  //二蓝
					paLedPtr[6 * 3 + 1] = 0xFF / 2;  //三红
					paLedPtr[6 * 3 + 2] = 0xFF / 2;  //三蓝
					paLedPtr[5 * 3 + 1] = 0xFF;  //四红
					paLedPtr[5 * 3 + 2] = 0xFF;  //四蓝
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
static void IRAM_ATTR key_isr_handler(void *arg)
{
	uint32_t gpio_num = (uint32_t) arg;
	eKey_t eKey = eKeyNone;
	uint8_t stateA = 0;
	uint8_t stateB = 0;
	switch (gpio_num)
	{
		case KEY_LEFT_IO_A:
			stateA = gpio_get_level(KEY_LEFT_IO_A);
			stateB = gpio_get_level(KEY_LEFT_IO_B);
			if (stateA)
			{
				if (stateB)
				{
					//顺时针
					eKey = eKeyLeftDec;
				}
				else
				{
					//逆时针
					eKey = eKeyLeftInc;
				}
			}
			else
			{
				if (stateB)
				{
					//逆时针
					eKey = eKeyLeftInc;
				}
				else
				{
					//顺时针
					eKey = eKeyLeftDec;
				}
			}
		break;
		case KEY_RIGHT_IO_A:
			stateA = gpio_get_level(KEY_RIGHT_IO_A);
			stateB = gpio_get_level(KEY_RIGHT_IO_B);
			if (stateA)
			{
				if (stateB)
				{
					//顺时针
					eKey = eKeyRightDec;
				}
				else
				{
					//逆时针
					eKey = eKeyRightInc;
				}
			}
			else
			{
				if (stateB)
				{
					//逆时针
					eKey = eKeyRightInc;
				}
				else
				{
					//顺时针
					eKey = eKeyRightDec;
				}
			}
		break;
		case KEY_UP_IO_A:
			stateA = gpio_get_level(KEY_UP_IO_A);
			stateB = gpio_get_level(KEY_UP_IO_B);
			if (stateA)
			{
				if (stateB)
				{
					//顺时针
					eKey = eKeyUpDec;
				}
				else
				{
					//逆时针
					eKey = eKeyUpInc;
				}
			}
		break;
		default:
			eKey = eKeyNone;
		break;
	}
	if (eKey != eKeyNone)
	{
		xQueueSendFromISR(*xKey_Queue, &eKey, NULL);
	}
}
#define GPIO_INPUT_PIN_SEL  ((1ULL<<KEY_LEFT_IO_A) | (1ULL<<KEY_RIGHT_IO_A)|(1ULL << KEY_UP_IO_A))
void vUI_Init(void)
{
	gpio_config_t io_conf;
	cicleled_init();
	gpio_set_direction(KEY_LEFT_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_LEFT_IO_B, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_LEFT_IO_BTN, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_RIGHT_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_RIGHT_IO_B, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_RIGHT_IO_BTN, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_UP_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_UP_IO_B, GPIO_MODE_INPUT);
	io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
	gpio_isr_handler_add(KEY_LEFT_IO_A, key_isr_handler, (void*) KEY_LEFT_IO_A);
	gpio_isr_handler_add(KEY_RIGHT_IO_A, key_isr_handler,
			(void*) KEY_RIGHT_IO_A);
	gpio_isr_handler_add(KEY_UP_IO_A, key_isr_handler, (void*) KEY_UP_IO_A);

//	return xKey_Queue;
}
