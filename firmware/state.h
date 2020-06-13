#include <stdint.h>

struct state {
	uint8_t mode;
	uint8_t level;
	int16_t batt_mv;
	uint8_t temp;
};

extern struct state state;
