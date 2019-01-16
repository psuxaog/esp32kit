/*
 * cicleled.c
 *
 *  Created on: 2018Äê12ÔÂ28ÈÕ
 *      Author: LinxSong
 */
#include <string.h>
#include "esp_system.h"
#include "driver/rmt.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RMT_TX_GPIO 21
static rmt_item32_t const over[] = { { { { 0, 1, 12000, 0 } } }, { { { 0, 1,
    12000, 0 } } } };

static void IRAM_ATTR u8_to_rmt(const void* src, rmt_item32_t* dest,
    size_t src_size, size_t wanted_num, size_t* translated_size,
    size_t* item_num)
{
	if (src == NULL || dest == NULL)
	{
		*translated_size = 0;
		*item_num = 0;
		return;
	}
	const rmt_item32_t bit0 = { { { 22, 1, 138, 0 } } }; //Logical 0
	const rmt_item32_t bit1 = { { { 72, 1, 88, 0 } } }; //Logical 1
	size_t size = 0;
	size_t num = 0;
	uint8_t *psrc = (uint8_t *) src;
	rmt_item32_t* pdest = dest;
	while (size < src_size && num < wanted_num)
	{
		for (int i = 0; i < 8; i++)
		{
			if (*psrc & (0x80 >> i))
			{
				pdest->val = bit1.val;
			}
			else
			{
				pdest->val = bit0.val;
			}
			num++;
			pdest++;
		}
		size++;
		psrc++;
	}
	*translated_size = size;
	*item_num = num;
}

/*
 * Initialize the RMT Tx channel
 */
void cicleled_init()
{
	rmt_config_t config;
	config.rmt_mode = RMT_MODE_TX;
	config.channel = RMT_TX_CHANNEL;
	config.gpio_num = RMT_TX_GPIO;
	config.mem_block_num = 1;
	config.tx_config.loop_en = 0;
	// enable the carrier to be able to hear the Morse sound
	// if the RMT_TX_GPIO is connected to a speaker
	config.tx_config.carrier_en = 0;
	config.tx_config.idle_output_en = 1;
	config.tx_config.idle_level = 0;
	config.tx_config.carrier_duty_percent = 50;
	// set audible career frequency of 611 Hz
	// actually 611 Hz is the minimum, that can be set
	// with current implementation of the RMT API
	config.tx_config.carrier_freq_hz = 611;
	config.tx_config.carrier_level = 1;
	// set the maximum clock divider to be able to output
	// RMT pulses in range of about one hundred milliseconds
	config.clk_div = 1;

	ESP_ERROR_CHECK(rmt_config(&config));
	ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
	ESP_ERROR_CHECK(rmt_translator_init(config.channel, u8_to_rmt));
}
void cicleled_write(uint8_t *dat, uint8_t len)
{
	ESP_ERROR_CHECK(rmt_write_sample(RMT_TX_CHANNEL, dat, len, true));
}
