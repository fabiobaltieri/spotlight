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

static void system_loop(void)
{
	maybe_shutdown();
}

static void system_thread(void)
{
	if (!device_is_ready(fuel_gauge)) {
		printk("fuel gauge device is not ready\n");
		return;
	}

	k_timer_start(&system_sync, K_SECONDS(1), K_SECONDS(1));

	for (;;) {
		system_loop();
		k_timer_status_sync(&system_sync);
	}
}

K_THREAD_DEFINE(system, 512, system_thread, NULL, NULL, NULL, 7, 0, 0);
