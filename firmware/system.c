#include <stdint.h>
#include "app_timer.h"
#include "boards.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_twi.h"
#include "nrf_drv_twi.h"
#include "nrf_log.h"

#include "mic280.h"
#include "state.h"
#include "telemetry.h"

#include "system.h"

// Temp sensor addresses (MIC280)
static uint8_t temps_addr[] = {0x4a, 0x4b};
static int8_t temps[] = {INT8_MIN, INT8_MIN};
static int8_t die_temp;

#define BATT_NUM (39420 / 2 / 2) // 3.6 * 10.95 * 1000
#define BATT_DEN (1024 / 2 / 2) // 10bit

APP_TIMER_DEF(temp_tmr);
static nrf_saadc_value_t adc_buf;
static const nrf_drv_twi_t twi = NRF_DRV_TWI_INSTANCE(0);

static int8_t get_max_temp(void)
{
	int8_t out = INT8_MIN;
	uint8_t i;

	if (!TARGET_HAS_EXT_TEMP)
		return die_temp;

	for (i = 0; i < sizeof(temps); i++)
		if (temps[i] > out)
			out = temps[i];

	return out;
}

static void saadc_callback(nrf_drv_saadc_evt_t const *evt)
{
	uint32_t adc_result;

	if (evt->type == NRF_DRV_SAADC_EVT_DONE) {
		adc_result = evt->data.done.p_buffer[0];
		state.batt_mv = (adc_result * BATT_NUM) / BATT_DEN;
	}
}

static void saadc_convert(void)
{
	ret_code_t err_code;

	err_code = nrf_drv_saadc_buffer_convert(&adc_buf, 1);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_sample();
	APP_ERROR_CHECK(err_code);
}

static void timer_handler(void *context)
{
	uint8_t i;
	int32_t temp;

	// Update the battery voltage (for the next sample)
	saadc_convert();

	// Update temperatures
	if (TARGET_HAS_EXT_TEMP)
		for (i = 0; i < sizeof(temps_addr); i++)
			temps[i] = mic280_read(&twi, temps_addr[i]);

	sd_temp_get(&temp);
	die_temp = temp / 4;

	state.temp = get_max_temp();

	// TODO: overtemperature protection

	telemetry_update();
}

static void twi_init(void)
{
	ret_code_t err_code;

	const nrf_drv_twi_config_t twi_config = {
		.scl                = TWI_SCL,
		.sda                = TWI_SDA,
		.frequency          = NRF_DRV_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init     = false
	};

	err_code = nrf_drv_twi_init(&twi, &twi_config, NULL, NULL);
	APP_ERROR_CHECK(err_code);

	nrf_drv_twi_enable(&twi);
}

void system_init(void)
{
	ret_code_t err_code;

	twi_init();

	/* Battery measurement ADC */

	nrf_saadc_channel_config_t channel_config =
		NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(BATTERY_SENSE_INPUT);

	err_code = nrf_drv_saadc_init(NULL, saadc_callback);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_channel_init(0, &channel_config);
	APP_ERROR_CHECK(err_code);

	saadc_convert();

	/* System status update cycle */

	err_code = app_timer_create(
			&temp_tmr, APP_TIMER_MODE_REPEATED, timer_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_start(temp_tmr, APP_TIMER_TICKS(1000), NULL);
	APP_ERROR_CHECK(err_code);
}
