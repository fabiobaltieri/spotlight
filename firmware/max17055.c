#include <stdint.h>
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"
#include "nrf_log.h"

#include "max17055.h"

#define MAX17055_ADDR 0x36

#define MAX17055_Status		0x00
#define MAX17055_RepCap		0x05
#define MAX17055_RepSOC		0x06
#define MAX17055_VCell		0x09
#define MAX17055_TTE		0x11
#define MAX17055_DesignCap	0x18
#define MAX17055_IChgTerm	0x1e
#define MAX17055_VEmpty		0x3a
#define MAX17055_FStat		0x3d
#define MAX17055_dQAcc		0x45
#define MAX17055_dPAcc		0x46
#define MAX17055_Command	0x60
#define MAX17055_HibCfg		0xba
#define MAX17055_ModelCfg	0xdb

#define STATUS_POR 0x0002
#define STATUS_POR_MASK 0xfffd
#define FSTAT_DNR 0x0001
#define MODELCFG_REFRESH 0x8000

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

	if (!(max17055_read(twi, MAX17055_Status) & STATUS_POR)) {
		return;
	}

	while (max17055_read(twi, MAX17055_FStat) & FSTAT_DNR) {
		nrf_delay_ms(30);
		bsp_board_led_invert(1);
	}

	hib_cfg = max17055_read(twi, MAX17055_HibCfg);
	max17055_write(twi, MAX17055_Command, 0x90);
	max17055_write(twi, MAX17055_HibCfg, 0x0);
	max17055_write(twi, MAX17055_Command, 0x0);

	uint16_t design_cap = MAX17055_CAP * 2;
	max17055_write(twi, MAX17055_DesignCap, design_cap);
	max17055_write(twi, MAX17055_dQAcc, design_cap / 32);
	max17055_write(twi, MAX17055_IChgTerm, MAX17055_ICHGTERM / 156.25 * 1000);
	max17055_write(twi, MAX17055_VEmpty, ((MAX17055_VE / 10) << 7) | (MAX17055_VR / 40));

	max17055_write(twi, MAX17055_dPAcc, 44138 / 32);
	max17055_write(twi, MAX17055_ModelCfg, MODELCFG_REFRESH);
	while (max17055_read(twi, MAX17055_ModelCfg) & MODELCFG_REFRESH) {
		nrf_delay_ms(30);
		bsp_board_led_invert(1);
	}

	max17055_write(twi, MAX17055_HibCfg, hib_cfg);

	status = max17055_read(twi, MAX17055_Status);
	max17055_write_verify(twi, MAX17055_Status, status & STATUS_POR_MASK);
}

uint16_t max17055_soc(const nrf_drv_twi_t *twi)
{
	return (max17055_read(twi, MAX17055_RepSOC) + 0x7f) >> 8;
}

uint16_t max17055_batt_mv(const nrf_drv_twi_t *twi)
{
	return max17055_read(twi, MAX17055_VCell) * 1.25 / 16;
}

uint16_t max17055_tte_mins(const nrf_drv_twi_t *twi)
{
	float tte;
	tte = max17055_read(twi, MAX17055_TTE) * 5.625 / 60;
	if (tte > UINT16_MAX)
		return UINT16_MAX;
	return tte;
}

