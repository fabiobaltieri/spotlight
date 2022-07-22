#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

static const struct device *leds;

void battery_blink(void)
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
	leds = DEVICE_DT_GET(DT_NODELABEL(leds));

	for (;;) {
		battery_blink();
		k_sleep(K_SECONDS(2));
	}
}
