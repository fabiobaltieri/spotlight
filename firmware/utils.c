#include <stdint.h>
#include <stdio.h>
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
