#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(levels);

static const struct device *pwm_leds = DEVICE_DT_GET_ONE(pwm_leds);

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
