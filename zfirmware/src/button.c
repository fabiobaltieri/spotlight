#include <hal/nrf_gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(button);

#include "input.h"

#define SW_DEBOUNCE_MS 100
#define SW_LONG_PRESS_MS 1000

#define SW0_NODE DT_NODELABEL(sw0)

static struct gpio_dt_spec sw0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback sw0_cb_data;
static struct k_sem sem;

static void button_pressed(const struct device *dev,
			   struct gpio_callback *cb, uint32_t pins)
{
	k_sem_give(&sem);
}

static void button_loop(void)
{
	int64_t ts;
	bool long_fired = false;

	k_sem_take(&sem, K_FOREVER);

	ts = k_uptime_get();

	k_sleep(K_MSEC(SW_DEBOUNCE_MS));

	while (gpio_pin_get_dt(&sw0)) {
		k_sleep(K_MSEC(SW_DEBOUNCE_MS));

		if (!long_fired &&
		    (k_uptime_get() - ts) >= SW_LONG_PRESS_MS) {
			long_fired = true;
			LOG_INF("long press");
			switch_long();
		}
	}

	if (!long_fired) {
		LOG_INF("short press");
		switch_short();
	}

	k_sem_reset(&sem);
}

static void button_thread(void)
{
	int ret;

	if (!device_is_ready(sw0.port)) {
		printk("SW0 device is not ready\n");
		return;
	}

	k_sem_init(&sem, 0, 1);

	ret = gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	if (ret) {
		printk("failed to configure sw0 gpio: %d\n", ret);
		return;
	}
	ret = gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		printk("failed to configure sw0 interrupt: %d\n", ret);
		return;
	}

	gpio_init_callback(&sw0_cb_data, button_pressed, BIT(sw0.pin));
	gpio_add_callback(sw0.port, &sw0_cb_data);

	nrf_gpio_cfg_sense_set(sw0.pin, NRF_GPIO_PIN_SENSE_LOW);

	for (;;) {
		button_loop();
	}
}

K_THREAD_DEFINE(button, 1024, button_thread, NULL, NULL, NULL, 7, 0, 0);
