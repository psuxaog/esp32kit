/*
 * bleSpeakerUi.h
 *
 *  Created on: 2019��1��5��
 *      Author: LinxSong
 */

#ifndef __KITUI_H__
#define __KITUI_H__
#include "esp_log.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#define WHEEL_IO_A    GPIO_NUM_22
#define WHEEL_IO_B    GPIO_NUM_23
#define WHEEL_IO_BTN  GPIO_NUM_21

#define KEY_IO    GPIO_NUM_13
typedef enum
{
	eKeyNone,
	eKeyWheelInc,
	eKeyWheelDec,
	eKeyWheelPress,
	eKeyWheelRelease,
	eKeyWheelLong,
	eKeyPress,
	eKeyRelease,
	eKeyLong,
	eKeyAllLong,
	eKeyAllPress,
	eKeyAllRelease,
} eKey_t;
extern void vUI_Task(void *parm); //10ms Call
extern void vUI_SetKeyQueue(xQueueHandle *queue);
extern void vUI_Init(void);
#endif /* EXAMPLESBUILD_PROTOCOLS_MQTT_TCP_MAIN_BLESPEAKERUI_H_ */
