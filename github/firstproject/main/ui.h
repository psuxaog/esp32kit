/*
 * bleSpeakerUi.h
 *
 *  Created on: 2019Äê1ÔÂ5ÈÕ
 *      Author: LinxSong
 */

#ifndef EXAMPLESBUILD_PROTOCOLS_MQTT_TCP_MAIN_BLESPEAKERUI_H_
#define EXAMPLESBUILD_PROTOCOLS_MQTT_TCP_MAIN_BLESPEAKERUI_H_
#include "esp_log.h"
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#define KEY_LEFT_IO_A    GPIO_NUM_4   //EC2_A
#define KEY_LEFT_IO_B    GPIO_NUM_27  //EC2_B
#define KEY_LEFT_IO_BTN  GPIO_NUM_33  //EC2_S

#define KEY_RIGHT_IO_A    GPIO_NUM_35  //EC1_A
#define KEY_RIGHT_IO_B    GPIO_NUM_32  //EC1_B
#define KEY_RIGHT_IO_BTN  GPIO_NUM_34  //EC1_S

#define KEY_UP_IO_A    GPIO_NUM_16  //ECV_A
#define KEY_UP_IO_B    GPIO_NUM_17  //ECV_B
typedef enum
{
	eKeyNone,
	eKeyLeftInc,
	eKeyLeftDec,
	eKeyLeftPress,
	eKeyLeftRelease,
	eKeyLeftLong,
	eKeyRightInc,
	eKeyRightDec,
	eKeyRightPress,
	eKeyRightRelease,
	eKeyRightLong,
	eKeyAllLong,
	eKeyAllPress,
	eKeyAllRelease,
	eKeyUpInc,
	eKeyUpDec,
} eKey_t;
#define LED_NUMS  8
typedef enum
{
	eLedModOff, eLedModWiFiStp1, eLedModWiFiStp2, eLedModWiFiCntOK, eLedModUser,
} eLedMod_t;
typedef struct
{
	uint32_t RGB[LED_NUMS];
	eLedMod_t eLedMod;
}sLed_t;
extern void vUI_Task(void *parm); //10ms Call
extern void vUI_SetKeyQueue(xQueueHandle *queue);
extern void vUI_SetLedQueue(xQueueHandle *queue);
extern void vUI_Init(void);
#endif /* EXAMPLESBUILD_PROTOCOLS_MQTT_TCP_MAIN_BLESPEAKERUI_H_ */
