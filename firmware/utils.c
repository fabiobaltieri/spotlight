#include <stdint.h>
#include "ant_channel_config.h"
#include "ant_parameters.h"
#include "app_error.h"
#include "nrf_log.h"

#include "utils.h"

void ant_dump_message(const char *id, uint8_t chan,uint8_t *payload)
{
	static char buf[24];
	memset(buf, 0, 24);
	snprintf(buf, 24, "%02x %02x %02x %02x %02x %02x %02x %02x",
			payload[0], payload[1], payload[2], payload[3],
			payload[4], payload[5], payload[6], payload[7]);

	NRF_LOG_INFO("%s (%d) [%s]", id, chan, buf)
}

void pwm_adjust_step(uint8_t *from, uint8_t to)
{
	uint8_t step;
	uint8_t delta;

	if (*from == to)
		return;

	if (*from > to)
		delta = *from - to;
	else
		delta = to - *from;

	if (delta < 10)
		step = 1;
	else if (delta < 30)
		step = 5;
	else
		step = 10;

	if (*from > to)
		*from -= step;
	else
		*from += step;
}
