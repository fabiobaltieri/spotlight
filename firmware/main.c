#include <stdbool.h>
#include <stdint.h>
#include "app_timer.h"
#include "app_pwm.h"
#include "boards.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_twi.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "mic280.h"
#include "remote.h"
#include "state.h"
#include "telemetry.h"
#include "utils.h"

#define DEBUG_ANT(a...) NRF_LOG_INFO(a)
#define DEBUG_LEVELS(a...) NRF_LOG_INFO(a)

// Temp sensor addresses (MIC280)
static uint8_t temps_addr[] = {0x4a, 0x4b};

// Running state
static uint8_t tgt_levels[] = {0, 0, 0, 0};
static uint8_t cur_levels[] = {1, 1, 1, 1};
static int8_t temps[] = {INT8_MIN, INT8_MIN};
static int8_t die_temp;
#define BATT_NUM (39420 / 2 / 2) // 3.6 * 10.95 * 1000
#define BATT_DEN (1024 / 2 / 2) // 10bit

// Module state
APP_PWM_INSTANCE(PWM1, 1);
APP_PWM_INSTANCE(PWM2, 2);
static const nrf_drv_twi_t twi = NRF_DRV_TWI_INSTANCE(0);
static nrf_saadc_value_t adc_buf;
APP_TIMER_DEF(temp_tmr);
APP_TIMER_DEF(pwm_tmr);
APP_TIMER_DEF(delay_tmr);

// Levels
static struct level {
	uint8_t a, b, c, d;
} levels[] = {
#if TARGET == TARGET_ACTIK
	/* S    O  Red   nc */
	{  0,   0,   0,   0}, // 0 - Off
	{  1,   0,   0,   0}, // 1 - Low
	{ 25,  25,   0,   0}, // 2 - Medium
	{100, 100,   0,   0}, // 3 - High
	{  0,   0, 100,   0}, // 4 - Red
#else
	/* S    O  O90    S */
	{  0,   0,   0,   0}, // 0 - Off
	{  0,   2,   0,   0}, // 1 - Low (150mW)
	{  4,  10,  10,   4}, // 2 - Medium (1.5W)
	{ 12,  38,  38,  12}, // 3 - High (5W)
	{ 60,   0,   0,  60}, // 4 - Beam (7W)
#endif
};

#define AUTO_CADENCE_TRESHOLD 142

struct state state;

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

void saadc_callback(nrf_drv_saadc_evt_t const *evt)
{
	uint32_t adc_result;

	if (evt->type == NRF_DRV_SAADC_EVT_DONE) {
		adc_result = evt->data.done.p_buffer[0];
		state.batt_mv = (adc_result * BATT_NUM) / BATT_DEN;
	}
}

void saadc_convert(void)
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

static void pwm_update(void)
{
	ret_code_t err_code;

	if (!TARGET_HAS_PWM)
		return;

	if (memcmp(tgt_levels, cur_levels, sizeof(tgt_levels)) == 0)
		return;

	err_code = app_timer_start(pwm_tmr, APP_TIMER_TICKS(20), NULL);
	APP_ERROR_CHECK(err_code);
}

static void pwm_timer_handler(void *context)
{
	pwm_adjust_step(&cur_levels[0], tgt_levels[0]);
	pwm_adjust_step(&cur_levels[1], tgt_levels[1]);
	pwm_adjust_step(&cur_levels[2], tgt_levels[2]);
	pwm_adjust_step(&cur_levels[3], tgt_levels[3]);

	while (app_pwm_channel_duty_set(&PWM1, 0, cur_levels[0]) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM1, 1, cur_levels[1]) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 0, cur_levels[2]) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 1, cur_levels[3]) == NRF_ERROR_BUSY);

	DEBUG_LEVELS("levels: %3d %3d %3d %3d",
			cur_levels[0], cur_levels[1],
			cur_levels[2], cur_levels[3]);

	pwm_update();
}

static void apply_state(uint8_t *manual)
{
	NRF_LOG_INFO("state mode: %d level: %d", state.mode, state.level);
	if (manual) {
		memcpy(tgt_levels, manual, sizeof(tgt_levels));
	} else {
		memcpy(tgt_levels, &levels[state.level], sizeof(tgt_levels));
	}
	pwm_update();
	telemetry_update();
}

void switch_remote(uint8_t *manual)
{
	state.mode = MODE_REMOTE;
	state.level = LEVEL_MEDIUM;
	apply_state(manual);
}

void switch_auto(uint8_t active, uint8_t speed, uint8_t cadence)
{
	uint8_t new_level;

	if (state.mode != MODE_AUTO) {
		NRF_LOG_INFO("RX discard, not in auto");
		return;
	}
	NRF_LOG_INFO("active: %d speed: %d cadence: %d",
			active, speed, cadence);

	if (!active)
		new_level = LEVEL_LOW;
	else if (cadence > AUTO_CADENCE_TRESHOLD)
		new_level = LEVEL_HIGH;
	else
		new_level = LEVEL_MEDIUM;

	if (new_level == state.level)
		return;

	state.level = new_level;
	apply_state(NULL);
}

static uint8_t switch_delay;

static void delay_timer_handler(void *context)
{
	switch_delay = 1;
	NRF_LOG_INFO("delay");
}

static void delay_timer_kick(void)
{
	ret_code_t err_code;

	switch_delay = 0;

	err_code = app_timer_stop(delay_tmr);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_start(delay_tmr, APP_TIMER_TICKS(2000), NULL);
	APP_ERROR_CHECK(err_code);
}

static void switch_short(void)
{
	if (state.mode == MODE_STANDBY)
		return;

	if (switch_delay && state.mode == MODE_MANUAL && state.level == LEVEL_HIGH) {
		state.level = LEVEL_MEDIUM;
	} else if (switch_delay && state.mode == MODE_MANUAL && state.level == LEVEL_BEAM) {
		state.level = LEVEL_MEDIUM;
	} else if (state.mode == MODE_MANUAL && state.level == LEVEL_BEAM) {
		state.mode = MODE_AUTO;
		state.level = LEVEL_LOW;
	} else if (state.mode == MODE_MANUAL) {
		state.level++;
	} else if (state.mode == MODE_AUTO) {
		state.mode = MODE_MANUAL;
		state.level = LEVEL_MEDIUM;
	} else if (state.mode == MODE_REMOTE) {
		state.mode = MODE_MANUAL;
		state.level = LEVEL_LOW;
	} else {
		NRF_LOG_INFO("I should not be here");
	}
	apply_state(NULL);

	delay_timer_kick();
}

static void switch_long(void)
{
	if (state.mode == MODE_STANDBY) {
		state.mode = MODE_MANUAL;
		state.level = LEVEL_LOW;
	} else {
		state.mode = MODE_STANDBY;
		state.level = LEVEL_OFF;
	}
	apply_state(NULL);
}

static void bsp_evt_handler(bsp_event_t event)
{
	switch (event) {
		case BSP_EVENT_KEY_0:
			switch_short();
			break;
		case BSP_EVENT_KEY_1:
			switch_long();
			break;
		default:
			break;
	}
}

static void hello(void)
{
	while (app_pwm_channel_duty_set(&PWM1, 0, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);
	while (app_pwm_channel_duty_set(&PWM1, 0, 0) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM1, 1, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);
	while (app_pwm_channel_duty_set(&PWM1, 1, 0) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 0, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);
	while (app_pwm_channel_duty_set(&PWM2, 0, 0) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 1, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);

	pwm_update();
}

static void timer_init(void)
{
	ret_code_t err_code;

	/* Temperature and voltage update cycle */
	err_code = app_timer_create(
			&temp_tmr, APP_TIMER_MODE_REPEATED, timer_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_start(temp_tmr, APP_TIMER_TICKS(1000), NULL);
	APP_ERROR_CHECK(err_code);

	/* PWM smoothing */
	err_code = app_timer_create(
			&pwm_tmr, APP_TIMER_MODE_SINGLE_SHOT, pwm_timer_handler);
	APP_ERROR_CHECK(err_code);

	/* Switch delay */
	err_code = app_timer_create(
			&delay_tmr, APP_TIMER_MODE_SINGLE_SHOT, delay_timer_handler);
	APP_ERROR_CHECK(err_code);
}

static void pwm_setup(void)
{
	ret_code_t err_code;

	if (!TARGET_HAS_PWM)
		return;

	app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(
			PWM_PERIOD_US, POWER_LED_1, POWER_LED_2);
	app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_2CH(
			PWM_PERIOD_US, POWER_LED_3, POWER_LED_4);

	pwm1_cfg.pin_polarity[0] = POWER_LED_POLARITY;
	pwm1_cfg.pin_polarity[1] = POWER_LED_POLARITY;
	pwm2_cfg.pin_polarity[0] = POWER_LED_POLARITY;
	pwm2_cfg.pin_polarity[1] = POWER_LED_POLARITY;

	err_code = app_pwm_init(&PWM1, &pwm1_cfg, NULL);
	APP_ERROR_CHECK(err_code);
	err_code = app_pwm_init(&PWM2, &pwm2_cfg, NULL);
	APP_ERROR_CHECK(err_code);

	app_pwm_enable(&PWM1);
	app_pwm_enable(&PWM2);
}

static void softdevice_setup(void)
{
	ret_code_t err_code;

	err_code = nrf_sdh_enable_request();
	APP_ERROR_CHECK(err_code);

	ASSERT(nrf_sdh_is_enabled());

	err_code = nrf_sdh_ant_enable();
	APP_ERROR_CHECK(err_code);
}

void saadc_init(void)
{
	ret_code_t err_code;

	nrf_saadc_channel_config_t channel_config =
		NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(BATTERY_SENSE_INPUT);

	err_code = nrf_drv_saadc_init(NULL, saadc_callback);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_channel_init(0, &channel_config);
	APP_ERROR_CHECK(err_code);

	saadc_convert();
}

static void utils_setup(void)
{
	ret_code_t err_code;

	err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);

	err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_evt_handler);
	APP_ERROR_CHECK(err_code);

	bsp_board_led_on(0);

	err_code = bsp_event_to_button_action_assign(
			0, BSP_BUTTON_ACTION_LONG_PUSH, BSP_EVENT_KEY_1);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
}

static void log_init(void)
{
	ret_code_t err_code;

	err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void)
{
	static uint16_t device_number;

	log_init();
	utils_setup();
	softdevice_setup();
	saadc_init();
	twi_init();
	timer_init();
	pwm_setup();

	bsp_board_led_on(1);

	hello();

	device_number = NRF_FICR->DEVICEADDR[0] & 0xffff;

	telemetry_setup(device_number);
	remote_setup();

	NRF_LOG_INFO("Started... devnum: %d", device_number);

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
