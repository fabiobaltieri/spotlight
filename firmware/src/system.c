#include <hal/nrf_gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/max17055.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>

LOG_MODULE_REGISTER(system);

#include "ble.h"
#include "levels.h"
#include "state.h"

static K_TIMER_DEFINE(system_sync, NULL, NULL);

#define SHUTDOWN_DELAY (60 * 15)

static const struct device *fuel_gauge = DEVICE_DT_GET_ONE(maxim_max17055);
static const struct device *temp = DEVICE_DT_GET_ONE(nordic_nrf_temp);

#define WKUP_PIN NRF_DT_GPIOS_TO_PSEL(DT_NODELABEL(wkup), gpios)

static void maybe_shutdown(void)
{
	static uint16_t shutdown_counter = SHUTDOWN_DELAY;

	if (state.mode != MODE_STANDBY) {
		shutdown_counter = SHUTDOWN_DELAY;
		return;
	}

	LOG_INF("shutdown_counter: %d", shutdown_counter);
	if (shutdown_counter--)
		return;

	LOG_INF("initiate shutdown");

	nrf_gpio_cfg_sense_set(WKUP_PIN, NRF_GPIO_PIN_SENSE_LOW);

	state.mode = MODE_SHUTDOWN;

	k_sleep(K_SECONDS(3));

	sys_poweroff();
}

#define RESERVE_MV 2800
static void maybe_reserve(void)
{
	if (state.level != LEVEL_HIGH)
		return;

	if (state.batt_mv > RESERVE_MV)
		return;

	state.level = LEVEL_MEDIUM;
	levels_apply_state();
}

static void fuel_gauge_update(void)
{
	struct sensor_value val;
	int32_t tte;

	sensor_sample_fetch_chan(fuel_gauge, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);
	sensor_sample_fetch_chan(fuel_gauge, SENSOR_CHAN_MAX17055_VFOCV);
	sensor_sample_fetch_chan(fuel_gauge, SENSOR_CHAN_GAUGE_TIME_TO_EMPTY);

	sensor_channel_get(fuel_gauge, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &val);
	state.soc = val.val1;

	sensor_channel_get(fuel_gauge, SENSOR_CHAN_MAX17055_VFOCV, &val);
	state.batt_mv = val.val1 * 1000 + val.val2 / 1000;

	sensor_channel_get(fuel_gauge, SENSOR_CHAN_GAUGE_TIME_TO_EMPTY, &val);
	tte = val.val1;
	if (tte > UINT8_MAX)
		state.tte = UINT8_MAX;
	else
		state.tte = tte;
}

static void temp_update(void)
{
	struct sensor_value val;

	sensor_sample_fetch(temp);

	sensor_channel_get(temp, SENSOR_CHAN_DIE_TEMP, &val);

	state.temp = val.val1;
}

#define R_BATT 0.36f
#define V_LED_EQ 2.70f
#define R_LED_EQ 0.50
#define R_SHUNT 0.7f
#define P_TARGET_HIGH 1.5f
#define P_TARGET_LOW 0.75f
static void dc_update(void)
{
	float vbatt;
	float i;
	float pled100;
	float dc;
	float p_target;

	if (state.level == LEVEL_HIGH)
		p_target = P_TARGET_HIGH;
	else
		p_target = P_TARGET_LOW;

	vbatt = state.batt_mv;
	i = (vbatt / 1000.0 - V_LED_EQ) / (R_BATT + R_LED_EQ + R_SHUNT);
	pled100 = (V_LED_EQ + R_LED_EQ * i) * i;
	dc = p_target / pled100 * 100;

	if (dc > 100)
		dc = 100;

	state.dc = dc;
}

static void system_loop(void)
{
	maybe_shutdown();
	maybe_reserve();
	fuel_gauge_update();
	temp_update();
	dc_update();
	ble_update();

	levels_apply_state();

}

static void system_thread(void)
{
	if (!device_is_ready(fuel_gauge)) {
		LOG_ERR("fuel gauge device is not ready");
		return;
	}

	if (!device_is_ready(temp)) {
		LOG_ERR("temp device is not ready");
		return;
	}

	k_timer_start(&system_sync, K_SECONDS(1), K_SECONDS(1));

	for (;;) {
		system_loop();
		k_timer_status_sync(&system_sync);
	}
}

K_THREAD_DEFINE(sys, 1024, system_thread, NULL, NULL, NULL, 7, 0, 0);
