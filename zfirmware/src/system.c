#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>

LOG_MODULE_REGISTER(system);

#include "state.h"

K_TIMER_DEFINE(system_sync, NULL, NULL);

//#define SHUTDOWN_DELAY (60 * 15)
#define SHUTDOWN_DELAY (60 * 1)

static const struct device *fuel_gauge = DEVICE_DT_GET_ONE(maxim_max17055);
static const struct device *temp = DEVICE_DT_GET_ANY(nordic_nrf_temp);

static void maybe_shutdown(void)
{
	static uint16_t shutdown_counter = SHUTDOWN_DELAY;
	struct pm_state_info pm_off = {PM_STATE_SOFT_OFF, 0, 0};

	if (state.mode != MODE_STANDBY) {
		shutdown_counter = SHUTDOWN_DELAY;
		return;
	}

	LOG_INF("shutdown_counter: %d", shutdown_counter);
	if (shutdown_counter--)
		return;

	LOG_INF("initiate shutdown");

	state.mode = MODE_SHUTDOWN;

	k_sleep(K_SECONDS(3));

	pm_state_force(0, &pm_off);
}

static void fuel_gauge_update(void)
{
	struct sensor_value val;
	uint16_t tte;

	sensor_sample_fetch(fuel_gauge);

	sensor_channel_get(fuel_gauge, SENSOR_CHAN_GAUGE_STATE_OF_CHARGE, &val);
	state.soc = val.val1;

	sensor_channel_get(fuel_gauge, SENSOR_CHAN_GAUGE_VOLTAGE, &val);
	state.batt_mv = val.val1 / 1000;

	sensor_channel_get(fuel_gauge, SENSOR_CHAN_GAUGE_TIME_TO_EMPTY, &val);
	tte = val.val1 / 1000 / 60;
	if (tte > 0xfe)
		state.tte = 0xfe;
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

static void system_loop(void)
{
	maybe_shutdown();
	fuel_gauge_update();
	temp_update();
}

static void system_thread(void)
{
	if (!device_is_ready(fuel_gauge)) {
		printk("fuel gauge device is not ready\n");
		return;
	}

	if (!device_is_ready(temp)) {
		printk("temp device is not ready\n");
		return;
	}

	k_timer_start(&system_sync, K_SECONDS(1), K_SECONDS(1));

	for (;;) {
		system_loop();
		k_timer_status_sync(&system_sync);
	}
}

K_THREAD_DEFINE(system, 512, system_thread, NULL, NULL, NULL, 7, 0, 0);
