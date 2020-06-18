#include <stdint.h>

enum {
	MODE_STANDBY = 0,
	MODE_MANUAL = 1,
	MODE_AUTO = 2,
	MODE_REMOTE = 3,
};

enum {
	LEVEL_OFF = 0,
	LEVEL_LOW = 1,
	LEVEL_MEDIUM = 2,
	LEVEL_HIGH = 3,
	LEVEL_BEAM = 4,
	NUM_LEVELS = 5,
};

struct state {
	uint8_t mode;
	uint8_t level;
	int16_t batt_mv;
	int16_t soc;
	uint8_t temp;
};

extern struct state state;
