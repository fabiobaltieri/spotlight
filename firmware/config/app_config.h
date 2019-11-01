#define APP_DEBUGGING_ON_PCA10040 1

#define NRF_SDH_ANT_TOTAL_CHANNELS_ALLOCATED 2

#define APP_PWM_ENABLED 1
#define NRFX_TIMER_ENABLED 1
#define NRFX_TIMER1_ENABLED 1
#define NRFX_TIMER2_ENABLED 1
#define NRFX_PPI_ENABLED 1

#define TWI_ENABLED 1
#define TWI0_ENABLED 1

#define NRFX_SAADC_ENABLED 1
#define NRFX_SAADC_CONFIG_RESOLUTION 1
#define NRFX_SAADC_CONFIG_OVERSAMPLE 0
#define NRFX_SAADC_CONFIG_IRQ_PRIORITY 6
#define NRFX_SAADC_CONFIG_LP_MODE 0

#define NRF_PWR_MGMT_CONFIG_DEBUG_PIN_ENABLED 0

#if APP_DEBUGGING_ON_PCA10040
#define NRF_LOG_BACKEND_UART_ENABLED 1
#else
#define NRF_LOG_BACKEND_UART_ENABLED 0
#endif
