#include <stdint.h>
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "levels.h"
#include "remote.h"
#include "state.h"
#include "system.h"
#include "telemetry.h"
#include "utils.h"

APP_TIMER_DEF(delay_tmr);

#define AUTO_CADENCE_TRESHOLD 142

struct state state;

void switch_remote(uint8_t *manual)
{
	state.mode = MODE_REMOTE;
	state.level = LEVEL_MEDIUM;
	levels_apply_state(manual);
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
	levels_apply_state(NULL);
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
	levels_apply_state(NULL);

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
	levels_apply_state(NULL);
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

static void timer_init(void)
{
	ret_code_t err_code;

	/* Switch delay */
	err_code = app_timer_create(
			&delay_tmr, APP_TIMER_MODE_SINGLE_SHOT, delay_timer_handler);
	APP_ERROR_CHECK(err_code);
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
	timer_init();
	system_init();
	levels_setup();

	bsp_board_led_on(1);

	levels_hello();

	device_number = NRF_FICR->DEVICEADDR[0] & 0xffff;

	telemetry_setup(device_number);
	remote_setup();

	NRF_LOG_INFO("Started... devnum: %d", device_number);

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
