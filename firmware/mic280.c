#include <stdint.h>
#include "nrf_drv_twi.h"
#include "nrf_log.h"

#include "mic280.h"

uint8_t mic280_read(const nrf_drv_twi_t *twi, uint8_t addr)
{
	ret_code_t err_code;
	uint8_t data;

	err_code = nrf_drv_twi_rx(twi, addr, &data, sizeof(data));
	if (err_code == NRF_SUCCESS) {
		return data;
	} else {
		return INT8_MIN;
	}
}
