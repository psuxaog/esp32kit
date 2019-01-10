/*
 * bleSpeakerUi.c
 *
 *  Created on: 2019��1��6��
 *      Author: LinxSong
 */

#include "kitui.h"
#include "string.h"
#define TAG "UITask"
static xQueueHandle *xKey_Queue;

static eKey_t eGetKey(void) {
	uint8_t valCur;
	static eKey_t eKey;
	static uint8_t valFilt = 0;
	static uint8_t valOld = 0;
	static uint8_t cntFilt = 0;
	static uint8_t oldPressed = 0;
	static uint8_t oldReleased = 0;
	static uint16_t cntPressed = 0;
	static uint8_t flagLongPressed = 0;
	if (!gpio_get_level(WHEEL_IO_BTN)) {
		valCur = 1;
	} else {
		valCur = 0;
	}
	if (!gpio_get_level(KEY_IO)) {
		valCur |= 2;
	}
	eKey = eKeyNone;
	if (valCur != valFilt) {
		cntFilt++;
		if (cntFilt >= 5) {
			valFilt = valCur;
			cntFilt = 0;
		}
	} else {
		cntFilt = 0;
	}
	if (valFilt != valOld) {
		if (!valFilt)    //�ɿ�
		{
			if (oldPressed) {
				if (!flagLongPressed) {
					if (valOld & 0x01) {
						eKey = eKeyWheelRelease;
					}
					if (valOld & 0x02) {
						eKey = eKeyRelease;
					}
					if ((valOld & 0x03) == 0x03) {
						eKey = eKeyAllRelease;
					}
				}
				flagLongPressed = 0;
			}
			oldPressed = 0;
		} else {
			oldPressed = 1;
			if (oldReleased) {
				if (valFilt & 0x01) {
					eKey = eKeyWheelPress;
				}
				if (valFilt & 0x02) {
					eKey = eKeyPress;
				}
				if ((valFilt & 0x03) == 0x03) {
					eKey = eKeyAllPress;
				}
			}
			oldReleased = 0;
		}
		valOld = valFilt;
	} else {
		if (valFilt) {
			if (cntPressed < 65535)
				cntPressed++;
			if (cntPressed == 200) {
				flagLongPressed = 1;
				if (valFilt & 0x01) {
					eKey = eKeyWheelLong;
				}
				if (valFilt & 0x02) {
					eKey = eKeyLong;
				}
				if ((valFilt & 0x03) == 0x03) {
					eKey = eKeyAllLong;
				}
			}
		} else {
			cntPressed = 0;
			oldReleased = 1;
		}
	}
	return eKey;
}
void vUI_Task(void *parm) {
	TickType_t xkeyTick;
	eKey_t eKey;
	uint8_t updated = 0;
	xkeyTick = xTaskGetTickCount();
	for (;;) {
		eKey = eGetKey();
		if (eKey != eKeyNone) {
			xQueueSend(*xKey_Queue, &eKey, NULL);
		}
		vTaskDelayUntil(&xkeyTick, (10 / portTICK_RATE_MS));
	}
}
void vUI_SetKeyQueue(xQueueHandle *queue) {
	xKey_Queue = queue;
}
static void IRAM_ATTR key_isr_handler(void *arg) {
	uint32_t gpio_num = (uint32_t) arg;
	eKey_t eKey = eKeyNone;
	uint8_t stateA = 0;
	uint8_t stateB = 0;
	if (gpio_num == WHEEL_IO_A) {
		stateA = gpio_get_level(WHEEL_IO_A);
		stateB = gpio_get_level(WHEEL_IO_B);
		if (stateA) {
			if (stateB) {
				eKey = eKeyWheelDec;
			} else {
				eKey = eKeyWheelInc;
			}
		} else {
			if (stateB) {
				eKey = eKeyWheelInc;
			} else {
				eKey = eKeyWheelDec;
			}
		}
	} else {
		eKey = eKeyNone;
	}
	if (eKey != eKeyNone) {
		//xQueueSendFromISR(*xKey_Queue, &eKey, NULL);
	}
}
#define GPIO_INPUT_PIN_SEL  (1ULL << WHEEL_IO_A)
void vUI_Init(void) {
	gpio_config_t io_conf;
	gpio_set_direction(WHEEL_IO_A, GPIO_MODE_INPUT);
	gpio_set_direction(WHEEL_IO_B, GPIO_MODE_INPUT);
	gpio_set_direction(WHEEL_IO_BTN, GPIO_MODE_INPUT);
	gpio_set_direction(KEY_IO, GPIO_MODE_INPUT);
	io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
	gpio_isr_handler_add(WHEEL_IO_A, key_isr_handler, (void*) WHEEL_IO_A);

//	return xKey_Queue;
}
