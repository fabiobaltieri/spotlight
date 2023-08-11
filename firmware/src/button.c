#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(button);

#include "input.h"

static const struct device *const input_dev = DEVICE_DT_GET(DT_NODELABEL(longpress));

static void input_cb(struct input_event *evt)
{
	if (evt->type != INPUT_EV_KEY) {
		return;
	}

	if (!evt->value) {
		return;
	}

	switch (evt->code) {
	case INPUT_KEY_A:
		switch_short();
		break;
	case INPUT_KEY_B:
		switch_long();
		break;
	default:
		LOG_INF("unknown code: %d", evt->code);
	}
}
INPUT_CALLBACK_DEFINE(input_dev, input_cb);
