#include <stdbool.h>
#include <stdint.h>
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "app_pwm.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

APP_PWM_INSTANCE(PWM1, 1);
APP_PWM_INSTANCE(PWM2, 2);

static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(0);

static void twi_read(uint8_t addr)
{
	ret_code_t err_code;
	uint8_t data;

	err_code = nrf_drv_twi_rx(&m_twi, addr, &data, sizeof(data));
	if (err_code == NRF_SUCCESS) {
		NRF_LOG_INFO("TWI: addr: %02x %02d", addr, data);
	} else {
		NRF_LOG_INFO("TWI: addr: %02x nope", addr);
	}
}

static void bsp_evt_handler(bsp_event_t event)
{
	NRF_LOG_INFO("event %02x", event);
	switch (event)
	{
		case BSP_EVENT_KEY_0:
			twi_read(0x4a);
			twi_read(0x4b);
			bsp_board_led_invert(0);
			NRF_LOG_INFO("Key 0");
			break;

		case BSP_EVENT_KEY_1:
			NRF_LOG_INFO("Key 1");
			break;

		case BSP_EVENT_KEY_2:
			NRF_LOG_INFO("Key 0 long");
			break;

		case BSP_EVENT_KEY_3:
			NRF_LOG_INFO("Key 1 long");
			break;

		default:
			break;
	}
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

	err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
	APP_ERROR_CHECK(err_code);

	nrf_drv_twi_enable(&m_twi);
}

APP_TIMER_DEF(test_tmr);
static void test_timer_handler(void *p_context)
{
	bsp_board_led_invert(1);
}

static void timer_init()
{
	ret_code_t err_code;
	err_code = app_timer_create(
			&test_tmr, APP_TIMER_MODE_REPEATED, test_timer_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_start(test_tmr, APP_TIMER_TICKS(1000), NULL);
	APP_ERROR_CHECK(err_code);
}

static void pwm_setup(void)
{
	ret_code_t err_code;

	app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(
			20L, POWER_LED_1, POWER_LED_2);
	app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_2CH(
			20L, POWER_LED_3, POWER_LED_4);

	pwm1_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;
	pwm1_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_LOW;
	pwm2_cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;
	pwm2_cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_LOW;

	err_code = app_pwm_init(&PWM1, &pwm1_cfg, NULL);
	APP_ERROR_CHECK(err_code);
	err_code = app_pwm_init(&PWM2, &pwm2_cfg, NULL);
	APP_ERROR_CHECK(err_code);

	app_pwm_enable(&PWM1);
	app_pwm_enable(&PWM2);

	while (app_pwm_channel_duty_set(&PWM1, 0, 5) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM1, 1, 5) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 0, 5) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 1, 5) == NRF_ERROR_BUSY);
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

	err_code = bsp_event_to_button_action_assign(
			0, BSP_BUTTON_ACTION_LONG_PUSH, BSP_EVENT_KEY_2);
	APP_ERROR_CHECK(err_code);

	err_code = bsp_event_to_button_action_assign(
			1, BSP_BUTTON_ACTION_LONG_PUSH, BSP_EVENT_KEY_3);
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
	log_init();
	utils_setup();
	softdevice_setup();
	pwm_setup();
	twi_init();
	timer_init();

	NRF_LOG_INFO("Started...");

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
