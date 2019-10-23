#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static void log_init(void)
{
	ret_code_t err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void)
{
	log_init();
	bsp_board_init(BSP_INIT_LEDS);
	bsp_board_led_on(0);

	NRF_LOG_INFO("Started...");
	NRF_LOG_FLUSH();

	while (true) {
		for (int i = 0; i < LEDS_NUMBER; i++) {
			bsp_board_led_invert(i);
			bsp_board_led_invert((i + 1) % LEDS_NUMBER);
			nrf_delay_ms(500);
		}
	}
}
