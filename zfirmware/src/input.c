#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/services/bas.h>

LOG_MODULE_REGISTER(input);

#include "input.h"
#include "levels.h"
#include "state.h"

static K_TIMER_DEFINE(delay, NULL, NULL);

void switch_short(void)
{
	bool delay_expired;

	if (state.mode == MODE_STANDBY)
		return;

	delay_expired = k_timer_status_get(&delay) > 0;

	if (delay_expired && state.mode == MODE_MANUAL && state.level == LEVEL_HIGH) {
		state.level = LEVEL_MEDIUM;
	} else if (delay_expired && state.mode == MODE_MANUAL && state.level == LEVEL_BEAM) {
		state.level = LEVEL_MEDIUM;
	} else if (state.mode == MODE_MANUAL && state.level == LEVEL_BEAM) {
		state.mode = MODE_AUTO;
		state.level = LEVEL_LOW;
	} else if (state.mode == MODE_MANUAL) {
		state.level++;
	} else if (state.mode == MODE_AUTO) {
		state.mode = MODE_MANUAL;
		state.level = LEVEL_MEDIUM;
	} else {
		LOG_ERR("I should not be here");
	}

	levels_apply_state();

	k_timer_start(&delay, K_SECONDS(2), K_NO_WAIT);
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

	levels_apply_state();
}

void switch_auto(uint8_t active)
{
	uint8_t new_level;

	if (state.mode != MODE_AUTO) {
		LOG_INF("RX discard, not in auto");
		return;
	}
	LOG_INF("active: %d ", active);

	if (active) {
		new_level = LEVEL_HIGH;
	} else {
		new_level = LEVEL_OFF;
	}

	if (new_level == state.level) {
		return;
	}

	state.level = new_level;

	levels_apply_state();
}
