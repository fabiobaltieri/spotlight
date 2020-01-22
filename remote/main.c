#include <stdbool.h>
#include <stdint.h>
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "ant_interface.h"
#include "ant_parameters.h"
#include "ant_channel_config.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define ANT_NETWORK_NUM 0
#define BROADCAST_CHANNEL_NUMBER 0
#define CHAN_ID_DEV_NUM 666
#define CHAN_ID_DEV_TYPE 0xfc
#define CHAN_ID_TRANS_TYPE 1
#define CHAN_PERIOD 16384
#define RF_FREQ 66

static struct config {
	uint8_t a, b, c, d;
} configs[] = {
	{0, 0, 0, 0},
	{10, 0, 0, 0},
	{0, 10, 0, 0},
	{0, 0, 10, 0},
	{0, 0, 0, 10},

	{0, 0, 0, 0},
	{0, 10, 0, 0},
	{0, 0, 10, 0},
	{0, 10, 10, 0},
	{0, 10, 0, 0},
	{5, 0, 0, 5},
	{5, 10, 0, 5},
	{5, 0, 10, 5},
	{5, 0, 0, 5},
	{5, 10, 10, 5},
};
static uint8_t config_idx;
#define NUM_CONFIGS (sizeof(configs) / sizeof(struct config))

static uint8_t scales[] = {1, 2, 5, 10};
static uint8_t scale_idx;
#define NUM_SCALES (sizeof scales)

static void ant_tx_load(void)
{
	uint8_t payload[ANT_STANDARD_DATA_PAYLOAD_SIZE];
	static uint8_t counter;

	memset(payload, 0, ANT_STANDARD_DATA_PAYLOAD_SIZE);

	payload[0] = configs[config_idx].a * scales[scale_idx];
	payload[1] = configs[config_idx].b * scales[scale_idx];
	payload[2] = configs[config_idx].c * scales[scale_idx];
	payload[3] = configs[config_idx].d * scales[scale_idx];

	payload[4] = config_idx;
	payload[5] = scale_idx;

	payload[7] = counter++;

	ret_code_t err_code = sd_ant_broadcast_message_tx(
			BROADCAST_CHANNEL_NUMBER,
			ANT_STANDARD_DATA_PAYLOAD_SIZE,
			payload);
	APP_ERROR_CHECK(err_code);
}

static void ant_evt_handler(ant_evt_t *ant_evt, void *context)
{
	if (ant_evt->channel != BROADCAST_CHANNEL_NUMBER)
		return;
	if (ant_evt->event != EVENT_TX)
		return;

	bsp_board_led_on(0);
	nrf_delay_ms(10);
	bsp_board_led_off(0);
}

#define ANT_OBSERVER_PRIO 1
NRF_SDH_ANT_OBSERVER(m_ant_observer, ANT_OBSERVER_PRIO, ant_evt_handler, NULL);

static void ant_channel_setup(void)
{
	ant_channel_config_t channel_config =
	{
		.channel_number    = BROADCAST_CHANNEL_NUMBER,
		.channel_type      = CHANNEL_TYPE_MASTER,
		.ext_assign        = 0x00,
		.rf_freq           = RF_FREQ,
		.transmission_type = CHAN_ID_TRANS_TYPE,
		.device_type       = CHAN_ID_DEV_TYPE,
		.device_number     = CHAN_ID_DEV_NUM,
		.channel_period    = CHAN_PERIOD,
		.network_number    = ANT_NETWORK_NUM,
	};

	ret_code_t err_code = ant_channel_init(&channel_config);
	APP_ERROR_CHECK(err_code);

	ant_tx_load();

	err_code = sd_ant_channel_open(BROADCAST_CHANNEL_NUMBER);
	APP_ERROR_CHECK(err_code);
}

static void bsp_evt_handler(bsp_event_t event)
{
	switch (event) {
		case BSP_EVENT_KEY_0:
			config_idx = (config_idx + 1) % NUM_CONFIGS;
			break;

		case BSP_EVENT_KEY_1:
			scale_idx = (scale_idx + 1) % NUM_SCALES;
			break;

		case BSP_EVENT_KEY_2:
			config_idx = (config_idx + NUM_CONFIGS - 1) % NUM_CONFIGS;
			break;

		case BSP_EVENT_KEY_4:
			bsp_board_led_on(1);
			nrf_delay_ms(100);
			bsp_board_led_off(1);

			bsp_wakeup_button_disable(1);
			nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
			break;

		default:
			break;
	}
	ant_tx_load();
}

static void softdevice_setup(void)
{
	ret_code_t err_code;

	err_code = nrf_sdh_enable_request();
	APP_ERROR_CHECK(err_code);

	ASSERT(nrf_sdh_is_enabled());

	err_code = nrf_sdh_ant_enable();
	APP_ERROR_CHECK(err_code);
}

static void utils_setup(void)
{
	ret_code_t err_code;

	err_code = app_timer_init();
	APP_ERROR_CHECK(err_code);

	err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_evt_handler);
	APP_ERROR_CHECK(err_code);

	err_code = bsp_event_to_button_action_assign(
			1, BSP_BUTTON_ACTION_LONG_PUSH, BSP_EVENT_KEY_4);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
}


static void log_init(void)
{
	ret_code_t err_code;

	err_code = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(err_code);

	NRF_LOG_DEFAULT_BACKENDS_INIT();
}

int main(void)
{
	log_init();
	utils_setup();
	softdevice_setup();

	ant_channel_setup();

	NRF_LOG_INFO("Started...");

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
