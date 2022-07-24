#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(levels);

#include "levels.h"
#include "state.h"

static const struct device *pwm_leds = DEVICE_DT_GET_ONE(pwm_leds);

static uint8_t tgt_levels[] = {0, 0, 0, 0};
static uint8_t cur_levels[] = {0, 0, 0, 0};

static struct level {
	uint8_t a, b, c, d;
} levels[] = {
	/* S    O  Red   nc */
	{  0,   0,   0,   0}, // 0 - Off
	{  1,   0,   0,   0}, // 1 - Low
	{ 25,  25,   0,   0}, // 2 - Medium
	{100, 100,   0,   0}, // 3 - High
	{  0,   0, 100,   0}, // 4 - Red
};

static void pwm_adjust_step(uint8_t *from, uint8_t to)
{
	uint8_t step;
	uint8_t delta;

	if (*from == to)
		return;

	if (*from > to)
		delta = *from - to;
	else
		delta = to - *from;

	if (delta < 10)
		step = 1;
	else if (delta < 30)
		step = 5;
	else
		step = 10;

	if (*from > to)
		*from -= step;
	else
		*from += step;
}

static void pwm_timer_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(pwm_work, pwm_timer_handler);

static void pwm_update(void)
{
	if (memcmp(tgt_levels, cur_levels, sizeof(tgt_levels)) == 0)
		return;

	k_work_reschedule(&pwm_work, K_MSEC(20));
}

static void pwm_timer_handler(struct k_work *work)
{
	pwm_adjust_step(&cur_levels[0], tgt_levels[0]);
	pwm_adjust_step(&cur_levels[1], tgt_levels[1]);
	pwm_adjust_step(&cur_levels[2], tgt_levels[2]);

	led_set_brightness(pwm_leds, 0, cur_levels[0]);
	led_set_brightness(pwm_leds, 1, cur_levels[1]);
	led_set_brightness(pwm_leds, 2, cur_levels[2]);

	LOG_INF("levels: %3d %3d %3d %3d",
		cur_levels[0], cur_levels[1],
		cur_levels[2], cur_levels[3]);

	pwm_update();
}

void levels_apply_state(void)
{
	LOG_INF("state mode: %d level: %d", state.mode, state.level);

	if (state.level == LEVEL_MEDIUM && state.level == LEVEL_HIGH) {
		tgt_levels[0] = state.dc;
		tgt_levels[1] = state.dc;
		tgt_levels[2] = 0;
		tgt_levels[3] = 0;
	} else {
		memcpy(tgt_levels, &levels[state.level], sizeof(tgt_levels));
	}

	pwm_update();
}

void levels_hello(void)
{
	led_set_brightness(pwm_leds, 0, 1);
	k_sleep(K_MSEC(100));
	led_set_brightness(pwm_leds, 0, 0);
	led_set_brightness(pwm_leds, 1, 1);
	k_sleep(K_MSEC(100));
	led_set_brightness(pwm_leds, 1, 0);
	led_set_brightness(pwm_leds, 2, 100);
	k_sleep(K_MSEC(100));
	led_set_brightness(pwm_leds, 2, 0);
}
