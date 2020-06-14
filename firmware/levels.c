#include <stdint.h>
#include "app_pwm.h"
#include "app_timer.h"
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_log.h"

#include "state.h"
#include "telemetry.h"

#include "levels.h"

#if TARGET_HAS_PWM

#define DEBUG_LEVELS(a...) NRF_LOG_INFO(a)

APP_PWM_INSTANCE(PWM1, 1);
APP_PWM_INSTANCE(PWM2, 2);

static uint8_t tgt_levels[] = {0, 0, 0, 0};
static uint8_t cur_levels[] = {1, 1, 1, 1};

APP_TIMER_DEF(pwm_tmr);

// Level definitions
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

static void pwm_update(void)
{
	ret_code_t err_code;

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

void levels_apply_state(uint8_t *manual)
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

void levels_hello(void)
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

void levels_setup(void)
{
	ret_code_t err_code;

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

	/* PWM smoothing */
	err_code = app_timer_create(
			&pwm_tmr, APP_TIMER_MODE_SINGLE_SHOT, pwm_timer_handler);
	APP_ERROR_CHECK(err_code);
}

#else
void levels_apply_state(uint8_t *manual)
{
}
void levels_hello(void)
{
}
void levels_setup(void)
{
}
#endif
