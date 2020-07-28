#include <stdint.h>
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"
#include "nrf_log.h"

#include "max17055.h"

#define MAX17055_ADDR 0x36

#define MAX17055_STATUS  0x00

static uint16_t max17055_read(const nrf_drv_twi_t *twi, uint8_t addr)
{
	ret_code_t err_code;
	uint8_t data[2];
	err_code = nrf_drv_twi_tx(twi, MAX17055_ADDR, &addr, sizeof(addr), true);
	APP_ERROR_CHECK(err_code);
	err_code = nrf_drv_twi_rx(twi, MAX17055_ADDR, data, sizeof(data));
	APP_ERROR_CHECK(err_code);

	return (data[1] << 8) | data[0] ;
}

static void max17055_write(const nrf_drv_twi_t *twi, uint8_t addr, uint16_t value)
{
	ret_code_t err_code;
	uint8_t data[3];
	data[0] = addr;
	data[1] = value & 0xff;
	data[2] = value >> 8;
	err_code = nrf_drv_twi_tx(twi, MAX17055_ADDR, data, sizeof(data), false);
	APP_ERROR_CHECK(err_code);
}

static void max17055_write_verify(const nrf_drv_twi_t *twi, uint8_t addr, uint16_t value)
{
	uint16_t readval;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		max17055_write(twi, addr, value);
		nrf_delay_ms(1);
		readval = max17055_read(twi, addr);
		if (readval == value)
			return;
	}
}

void max17055_init(const nrf_drv_twi_t *twi)
{
	uint16_t hib_cfg;
	uint16_t status;

	if (max17055_read(twi, 0x00) & 0x0002) { // status POR
		return;
	}

	while (max17055_read(twi, 0x3d) & 0x0001) { // fstat DNR
		nrf_delay_ms(30);
		bsp_board_led_invert(1);
	}

	hib_cfg = max17055_read(twi, 0xba);
	max17055_write(twi, 0x60, 0x90);
	max17055_write(twi, 0xba, 0x0);
	max17055_write(twi, 0x60, 0x0);

	uint16_t design_cap = 1250 * 2;
	max17055_write(twi, 0x18, design_cap);
	max17055_write(twi, 0x45, design_cap / 32);
	max17055_write(twi, 0x1e, 0x0640);
	max17055_write(twi, 0x3a, (280 << 7) | 61);

	max17055_write(twi, 0x46, 44138 / 32);
	max17055_write(twi, 0xdb, 0x8000);
	while (max17055_read(twi, 0xdb) & 0x8000) { // cfg refresh
		nrf_delay_ms(30);
		bsp_board_led_invert(1);
	}

	max17055_write(twi, 0xba, hib_cfg);

	status = max17055_read(twi, 0x00);
	max17055_write_verify(twi, 0x00, status & 0xfffd);
}

uint16_t max17055_soc(const nrf_drv_twi_t *twi)
{
	return (max17055_read(twi, 0x06) + 0x7f) >> 8;
}

uint16_t max17055_batt_mv(const nrf_drv_twi_t *twi)
{
	return max17055_read(twi, 0x09) * 1.25 / 16;
}

uint16_t max17055_tte_mins(const nrf_drv_twi_t *twi)
{
	float tte;
	tte = max17055_read(twi, 0x11) * 5.625 / 60;
	if (tte > 0xffff)
		return 0xffff;
	return tte;
}

