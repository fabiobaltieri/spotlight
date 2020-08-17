#include <stdint.h>
#include "app_timer.h"
#include "boards.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_twi.h"
#include "nrf_log.h"
#include "nrf_pwr_mgmt.h"

#include "levels.h"
#include "max17055.h"
#include "mic280.h"
#include "state.h"
#include "telemetry.h"

#include "system.h"

#define BATT_NUM (13200 / 2 / 2) // 1.2 * 11 * 1000
#define BATT_DEN (1024 / 2 / 2) // 10bit

#define SHUTDOWN_DELAY (60 * 15)

APP_TIMER_DEF(sys_tmr);
static const nrf_drv_twi_t twi = NRF_DRV_TWI_INSTANCE(SYSTEM_TWI_INSTANCE);

static void maybe_shutdown(void)
{
        static uint16_t shutdown_counter = SHUTDOWN_DELAY;
        ret_code_t err_code;

        if (state.mode != MODE_STANDBY) {
                shutdown_counter = SHUTDOWN_DELAY;
                return;
        }

        NRF_LOG_INFO("shutdown_counter: %d", shutdown_counter);
        if (shutdown_counter--)
               return;

        NRF_LOG_INFO("initiate shutdown");

        err_code = app_timer_stop_all();
        APP_ERROR_CHECK(err_code);

	bsp_board_led_off(0);
	bsp_board_led_off(1);

        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
}

#ifndef TARGET_HAS_FUEL_GAUGE
static nrf_saadc_value_t adc_buf;

static uint8_t level_changed(void)
{
	static uint8_t old_level;

	if (old_level == state.level)
		return 0;

	old_level = state.level;

	return 1;
}

#define PEAK_VALS 10
static uint8_t peak(uint8_t in)
{
	static uint8_t vals[PEAK_VALS];
	static uint8_t idx;
	uint8_t i;
	uint8_t out;

	vals[idx] = in;
	idx = (idx + 1) % PEAK_VALS;

	out = 0;
	for (i = 0; i < PEAK_VALS; i++)
		if (vals[i] > out)
			out = vals[i];

	return out;
}

static void soc_update(uint16_t batt_mv)
{
	uint16_t full, empty;
	int32_t soc;

	if (level_changed())
		return;

	if (state.level == LEVEL_HIGH || state.level == LEVEL_BEAM) {
		full = 7700;
		empty = 5900;
	} else {
		full = 8200;
		empty = 6000;
	}

	soc = (batt_mv - empty) * 100 / (full - empty);
	if (soc > 100)
		soc = 100;
	else if (soc < 0)
		soc = 0;

	state.soc = peak(soc);
	state.tte = 0xff;
}

static void saadc_callback(nrf_drv_saadc_evt_t const *evt)
{
	uint32_t adc_result;

	if (evt->type == NRF_DRV_SAADC_EVT_DONE) {
		adc_result = evt->data.done.p_buffer[0];
		state.batt_mv = (adc_result * BATT_NUM) / BATT_DEN;
		soc_update(state.batt_mv);
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

static void saadc_init(void)
{
	ret_code_t err_code;

	nrf_saadc_channel_config_t channel_config =
		NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(BATTERY_SENSE_INPUT);
	channel_config.gain = NRF_SAADC_GAIN1_2;

	err_code = nrf_drv_saadc_init(NULL, saadc_callback);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_channel_init(0, &channel_config);
	APP_ERROR_CHECK(err_code);

	saadc_convert();
}

#else
static void saadc_convert(void) {}
static void saadc_init(void) {}
#endif

static void maybe_reserve(void)
{
	if (state.level != LEVEL_HIGH)
		return;

#ifdef MAX17055_VE
	if (state.batt_mv > MAX17055_VE)
		return;
#else
	if (state.soc > 0)
		return;
#endif

	state.level = LEVEL_MEDIUM;
	levels_apply_state(NULL);
}

#ifdef TARGET_HAS_EXT_TEMP
#define MIC280_ADDR_A 0x4a
#define MIC280_ADDR_B 0x4b
static void temp_update(void)
{
	uint8_t t1, t2;
	t1 = mic280_read(&twi, MIC280_ADDR_A);
	t2 = mic280_read(&twi, MIC280_ADDR_B);

	if (t1 > t2)
		state.temp = t1;
	else
		state.temp = t2;
}
#else
static void temp_update(void)
{
	int32_t temp;
	sd_temp_get(&temp);
	state.temp = temp / 4;
}
#endif

#ifdef TARGET_HAS_FUEL_GAUGE
static void fuel_gauge_update(void)
{
	uint16_t tte;

	state.soc = max17055_soc(&twi);
	state.batt_mv = max17055_batt_mv(&twi);
	tte = max17055_tte_mins(&twi);
	if (tte > 0xfe)
		state.tte = 0xfe;
	else
		state.tte = tte;
}

static void fuel_gauge_init(void)
{
	max17055_init(&twi);
}
#else
static void fuel_gauge_update(void) {}
static void fuel_gauge_init(void) {}
#endif

static void timer_handler(void *context)
{

	maybe_shutdown();
	maybe_reserve();
	temp_update();
	saadc_convert();
	fuel_gauge_update();

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
	saadc_init();
	fuel_gauge_init();

	/* System status update cycle */

	err_code = app_timer_create(
			&sys_tmr, APP_TIMER_MODE_REPEATED, timer_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_start(sys_tmr, APP_TIMER_TICKS(1000), NULL);
	APP_ERROR_CHECK(err_code);
}
