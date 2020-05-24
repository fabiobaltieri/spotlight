#include <stdint.h>
#include "boards.h"
#include "nrf_mbr.h"
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_dfu.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"

static void on_error(void)
{
	NRF_LOG_FINAL_FLUSH();
	NVIC_SystemReset();
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
	on_error();
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
	NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
	on_error();
}

void app_error_handler_bare(uint32_t error_code)
{
	NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
	on_error();
}

static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
	switch (evt_type) {
		case NRF_DFU_EVT_DFU_FAILED:
		case NRF_DFU_EVT_DFU_ABORTED:
		case NRF_DFU_EVT_DFU_INITIALIZED:
			bsp_board_init(BSP_INIT_LEDS);
			bsp_board_led_on(BSP_BOARD_LED_0);
			bsp_board_led_on(BSP_BOARD_LED_1);
			bsp_board_led_off(BSP_BOARD_LED_2);
			break;
		case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
			bsp_board_led_off(BSP_BOARD_LED_1);
			bsp_board_led_on(BSP_BOARD_LED_2);
			break;
		case NRF_DFU_EVT_DFU_STARTED:
			break;
		default:
			break;
	}
}


int main(void)
{
	uint32_t ret_val;

	// Must happen before flash protection is applied, since it edits a protected page.
	nrf_bootloader_mbr_addrs_populate();

	// Protect MBR and bootloader code from being overwritten.
	ret_val = nrf_bootloader_flash_protect(0, MBR_SIZE, false);
	APP_ERROR_CHECK(ret_val);
	ret_val = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, BOOTLOADER_SIZE, false);
	APP_ERROR_CHECK(ret_val);

	(void) NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
	NRF_LOG_DEFAULT_BACKENDS_INIT();

	NRF_LOG_INFO("Inside main");

	ret_val = nrf_bootloader_init(dfu_observer);
	APP_ERROR_CHECK(ret_val);

	NRF_LOG_FLUSH();

	NRF_LOG_ERROR("After main, should never be reached.");
	NRF_LOG_FLUSH();

	APP_ERROR_CHECK_BOOL(false);
}
