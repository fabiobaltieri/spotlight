#include <stdint.h>
#include "app_timer.h"
#include "boards.h"
#include "nrf_log.h"

#include "levels.h"
#include "state.h"
#include "system.h"

APP_TIMER_DEF(delay_tmr);
static uint8_t switch_delay;

#define AUTO_CADENCE_TRESHOLD 142

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

void switch_short(void)
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

void switch_long(void)
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

void input_init(void)
{
	ret_code_t err_code;

	/* Switch delay */
	err_code = app_timer_create(
			&delay_tmr, APP_TIMER_MODE_SINGLE_SHOT, delay_timer_handler);
	APP_ERROR_CHECK(err_code);
}
