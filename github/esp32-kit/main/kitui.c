/*
 * bleSpeakerUi.c
 *
 *  Created on: 2019��1��6��
 *      Author: LinxSong
 */

#include "kitui.h"
#include "string.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_system.h"
#define TAG "UITask"
static xQueueHandle *xKey_Queue;
esp_timer_handle_t timerHandle = NULL;
static void timerPeriodicCb(void *arg);
esp_timer_create_args_t timerPeriodicArg =
{ .callback = &timerPeriodicCb, .arg = NULL, .name = "Timer" };
#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (3.4179) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   (5.78)   // sample test interval for the second timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

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
	if (!gpio_get_level(WHEEL_IO_BTN))
	{
		valCur = 1;
	}
	else
	{
		valCur = 0;
	}
	if (!gpio_get_level(KEY_IO))
	{
		valCur |= 2;
	}
	eKey = eKeyNone; // @suppress("Symbol is not resolved")
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
						eKey = eKeyWheelRelease;
					}
					if (valOld & 0x02)
					{
						eKey = eKeyRelease;
					}
					if ((valOld & 0x03) == 0x03)
					{
						eKey = eKeyAllRelease; // @suppress("Symbol is not resolved")
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
					eKey = eKeyWheelPress;
				}
				if (valFilt & 0x02)
				{
					eKey = eKeyPress;
				}
				if ((valFilt & 0x03) == 0x03)
				{
					eKey = eKeyAllPress; // @suppress("Symbol is not resolved")
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
					eKey = eKeyWheelLong;
				}
				if (valFilt & 0x02)
				{
					eKey = eKeyLong;
				}
				if ((valFilt & 0x03) == 0x03)
				{
					eKey = eKeyAllLong; // @suppress("Symbol is not resolved")
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
void vUI_Task(void *parm)
{
	TickType_t xkeyTick;
	eKey_t eKey;
	xkeyTick = xTaskGetTickCount();
	for (;;)
	{
		eKey = eGetKey();
		if (eKey != eKeyNone) // @suppress("Symbol is not resolved")
		{
			xQueueSend(*xKey_Queue, &eKey, NULL);
		}
		vTaskDelayUntil(&xkeyTick, (10 / portTICK_RATE_MS)); // @suppress("Symbol is not resolved")
	}
}
void vUI_SetKeyQueue(xQueueHandle *queue)
{
	xKey_Queue = queue;
}
eKey_t const eKeys[] =
{ eKeyWheelInc, eKeyWheelDec, eKeyWheelDec, eKeyWheelInc, };
static void timerPeriodicCb(void *arg)
{
	eKey_t eKey = eKeyNone; // @suppress("Symbol is not resolved")
	static uint8_t cntFilter = 0;
	static uint8_t keyOld = 0;
	uint8_t keyCur = 0;
	keyCur = gpio_get_level(WHEEL_IO_B);
	keyCur <<= 1;
	keyCur |= gpio_get_level(WHEEL_IO_A);
	if (keyCur != keyOld)
	{
		cntFilter++;
		if (cntFilter > 10)
		{
			if ((keyOld & 0x01) != (keyCur & 0x01))
			{
				eKey = eKeys[keyOld & 0x03];
				if (eKey != eKeyNone) // @suppress("Symbol is not resolved")
				{
					xQueueSendFromISR(*xKey_Queue, &eKey, NULL);
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
void vUI_Init(void)
{
	gpio_set_direction(WHEEL_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(WHEEL_IO_B, GPIO_MODE_INPUT);
	gpio_set_direction(WHEEL_IO_BTN, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_IO, GPIO_MODE_INPUT);
	ESP_ERROR_CHECK(esp_timer_create(&timerPeriodicArg, &timerHandle));
	ESP_ERROR_CHECK(esp_timer_start_periodic(timerHandle, 100));
}
