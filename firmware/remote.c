#include <stdint.h>
#include "boards.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "ant_interface.h"
#include "ant_parameters.h"
#include "ant_channel_config.h"

#include "nrf_log.h"

#include "state.h"
#include "utils.h"

#define DEBUG_ANT(a...) NRF_LOG_INFO(a)

// Remote Slave channel
#define REMOTE_REOPEN 0
#define REMOTE_CHANNEL 1
#define REMOTE_ANT_NETWORK_NUM 0
#define REMOTE_CHAN_ID_DEV_NUM 0
#define REMOTE_CHAN_ID_DEV_TYPE 0xfc
#define REMOTE_CHAN_ID_TRANS_TYPE 0
#define REMOTE_CHAN_PERIOD 16384
#define REMOTE_RF_FREQ 66

static void remote_rx_process(uint8_t *payload)
{
	uint8_t tgt[4];
	ant_dump_message("RX", REMOTE_CHANNEL, payload);

	tgt[0] = payload[0];
	tgt[1] = payload[1];
	tgt[2] = payload[2];
	tgt[3] = payload[3];

	switch_remote(tgt);
}

static void ant_evt_remote(ant_evt_t *ant_evt, void *context)
{
#if REMOTE_REOPEN
	ret_code_t err_code;
#endif
	uint8_t channel = ant_evt->channel;

	if (ant_evt->channel != REMOTE_CHANNEL)
		return;

	bsp_board_led_invert(0);

	switch (ant_evt->event) {
		case EVENT_RX:
			remote_rx_process(ant_evt->message.ANT_MESSAGE_aucPayload);
			break;
		case EVENT_RX_SEARCH_TIMEOUT:
		case EVENT_RX_FAIL_GO_TO_SEARCH:
			break;
		case EVENT_CHANNEL_CLOSED:
			DEBUG_ANT("ANT %d: channel closed", channel);
			bsp_board_led_off(0);
#if REMOTE_REOPEN
			err_code = sd_ant_channel_open(channel);
			APP_ERROR_CHECK(err_code);
#endif
			break;
		default:
			DEBUG_ANT("ANT event %d %02x",
					ant_evt->channel, ant_evt->event);
			break;
	}
}

NRF_SDH_ANT_OBSERVER(m_ant_observer, 1, ant_evt_remote, NULL);

void remote_setup(void)
{
	ret_code_t err_code;

	if (!TARGET_HAS_REMOTE)
		return;

	/* Remote */
	ant_channel_config_t r_channel_config = {
		.channel_number    = REMOTE_CHANNEL,
		.channel_type      = CHANNEL_TYPE_SLAVE,
		.ext_assign        = 0x00,
		.rf_freq           = REMOTE_RF_FREQ,
		.transmission_type = REMOTE_CHAN_ID_TRANS_TYPE,
		.device_type       = REMOTE_CHAN_ID_DEV_TYPE,
		.device_number     = REMOTE_CHAN_ID_DEV_NUM,
		.channel_period    = REMOTE_CHAN_PERIOD,
		.network_number    = REMOTE_ANT_NETWORK_NUM,
	};

	err_code = ant_channel_init(&r_channel_config);
	APP_ERROR_CHECK(err_code);

	err_code = sd_ant_channel_open(REMOTE_CHANNEL);
	APP_ERROR_CHECK(err_code);
}

