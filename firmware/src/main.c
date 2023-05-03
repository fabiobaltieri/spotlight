#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#include "levels.h"
#include "state.h"

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf52_dk_nrf52832)
#warning "building for nRF52DK"
#endif

struct state state;

static const struct device *leds = DEVICE_DT_GET_ONE(gpio_leds);

static K_TIMER_DEFINE(blink_sync, NULL, NULL);

static void battery_blink(void)
{
	int i;
	int blinks;

	blinks = CLAMP((state.soc / 25) + 1, 1, 4);
	for (i = 0; i < blinks; i++) {
		led_on(leds, 0);
		k_sleep(K_MSEC(10));
		led_off(leds, 0);
		k_sleep(K_MSEC(300));
	}
}

void main(void)
{
	if (!device_is_ready(leds)) {
		LOG_ERR("LED device is not ready");
		return;
	}

	levels_hello();

	k_timer_start(&blink_sync, K_SECONDS(3), K_SECONDS(3));

	for (;;) {
		if (state.mode != MODE_SHUTDOWN) {
			battery_blink();
		}
		k_timer_status_sync(&blink_sync);
	}
}
