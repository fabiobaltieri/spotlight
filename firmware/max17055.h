void max17055_init(const nrf_drv_twi_t *twi);
uint16_t max17055_soc(const nrf_drv_twi_t *twi);
uint16_t max17055_batt_mv(const nrf_drv_twi_t *twi);
int32_t max17055_batt_i(const nrf_drv_twi_t *twi);
uint8_t max17055_temp(const nrf_drv_twi_t *twi);
uint16_t max17055_tte_mins(const nrf_drv_twi_t *twi);
uint16_t max17055_ttf_mins(const nrf_drv_twi_t *twi);
