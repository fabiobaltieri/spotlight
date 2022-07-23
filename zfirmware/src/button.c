#include <hal/nrf_gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

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
	int ts;
	bool long_fired = false;

	k_sem_take(&sem, K_FOREVER);

	ts = k_uptime_get_32();

	k_sleep(K_MSEC(SW_DEBOUNCE_MS));

	while (gpio_pin_get_dt(&sw0)) {
		k_sleep(K_MSEC(SW_DEBOUNCE_MS));

		if (!long_fired &&
		    (k_uptime_get_32() - ts) >= SW_LONG_PRESS_MS) {
			long_fired = true;
			printk("long\n"); /* TODO: long handler */
		}
	}

	if (!long_fired) {
		printk("short\n"); /* TODO: short handler */
	}

	k_sem_reset(&sem);
}

static void button_thread(void)
{
	k_sem_init(&sem, 0, 1);

	gpio_pin_configure_dt(&sw0, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&sw0, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&sw0_cb_data, button_pressed, BIT(sw0.pin));
	gpio_add_callback(sw0.port, &sw0_cb_data);

	nrf_gpio_cfg_sense_set(sw0.pin, NRF_GPIO_PIN_SENSE_LOW);

	for (;;) {
		button_loop();
	}
}

K_THREAD_DEFINE(button, 512, button_thread, NULL, NULL, NULL, 7, 0, 0);
