#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include "levels.h"
#include "state.h"

struct state state;

static const struct device *leds = DEVICE_DT_GET_ONE(gpio_leds);

static K_TIMER_DEFINE(blink_sync, NULL, NULL);

static void battery_blink(void)
{
	int i;
	int soc = 100; /* TODO: read actual SoC */
	int blinks;

	blinks = CLAMP((soc / 25) + 1, 1, 4);
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
		printk("LED device is not ready\n");
		return;
	}

	levels_hello();

	k_timer_start(&blink_sync, K_SECONDS(3), K_SECONDS(3));

	for (;;) {
		if (state.mode == MODE_STANDBY) {
			battery_blink();
		}
		k_timer_status_sync(&blink_sync);
	}
}
