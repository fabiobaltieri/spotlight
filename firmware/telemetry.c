#include <stdint.h>
#include "ant_channel_config.h"
#include "ant_interface.h"
#include "ant_parameters.h"
#include "boards.h"
#include "nrf_log.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "input.h"
#include "state.h"
#include "utils.h"

#include "telemetry.h"

#define DEBUG_ANT(a...) NRF_LOG_INFO(a)

// Telemetry Master channel
#define TELEMETRY_CHANNEL 0
#define TELEMETRY_ANT_NETWORK_NUM 0
#define TELEMETRY_CHAN_ID_DEV_TYPE 0x7b
#define TELEMETRY_CHAN_ID_TRANS_TYPE 5
#define TELEMETRY_CHAN_PERIOD 16384
#define TELEMETRY_RF_FREQ 48

#define PAGE_0 0x00
#define PAGE_16 0x10

static uint8_t payload_unchanged(uint8_t *new)
{
	static uint8_t old[ANT_STANDARD_DATA_PAYLOAD_SIZE];

	if (memcmp(old, new, ANT_STANDARD_DATA_PAYLOAD_SIZE) == 0)
		return 1;

	memcpy(old, new, ANT_STANDARD_DATA_PAYLOAD_SIZE);

	return 0;
}

void telemetry_update(void)
{
	ret_code_t err_code;
	uint8_t payload[ANT_STANDARD_DATA_PAYLOAD_SIZE];

	payload[0] = PAGE_0; // Page 0

	payload[1] = state.mode | (state.level << 4); // Mode + Level
	payload[2] = state.soc; // State of charge
	payload[3] = state.temp; // Temperature (C)
	payload[4] = state.tte; // Time to empty
	payload[5] = 0xff;
	payload[6] = state.batt_mv & 0xff; // Battery voltage (mV)
	payload[7] = state.batt_mv >> 8;

	if (payload_unchanged(payload))
		return;

	err_code = sd_ant_broadcast_message_tx(
			TELEMETRY_CHANNEL,
			ANT_STANDARD_DATA_PAYLOAD_SIZE,
			payload);
	APP_ERROR_CHECK(err_code);

	ant_dump_message("TX", TELEMETRY_CHANNEL, payload);
}

static void telemetry_rx_process(uint8_t *payload)
{
	uint8_t active;
	uint8_t speed;
	uint8_t cadence;

	ant_dump_message("RX", TELEMETRY_CHANNEL, payload);

	if (payload[0] != PAGE_16) {
		return;
	}

	active = payload[1];
	speed = payload[2];
	cadence = payload[3];

	switch_auto(active, speed, cadence);
}

static void ant_evt_telemetry(ant_evt_t *ant_evt, void *context)
{
	uint8_t channel = ant_evt->channel;

	if (ant_evt->channel != TELEMETRY_CHANNEL)
		return;

	switch (ant_evt->event) {
		case EVENT_TX:
			break;
		case EVENT_RX:
			telemetry_rx_process(ant_evt->message.ANT_MESSAGE_aucPayload);
			break;
		case EVENT_CHANNEL_COLLISION:
			DEBUG_ANT("ANT %d: channel collision", channel);
			break;
		default:
			DEBUG_ANT("ANT event %d %02x",
					ant_evt->channel, ant_evt->event);
			break;
	}
}

NRF_SDH_ANT_OBSERVER(m_ant_observer, 1, ant_evt_telemetry, NULL);

void telemetry_setup(uint16_t device_number)
{
	ret_code_t err_code;

	/* Telemetry */
	ant_channel_config_t t_channel_config = {
		.channel_number    = TELEMETRY_CHANNEL,
		.channel_type      = CHANNEL_TYPE_MASTER,
		.ext_assign        = 0x00,
		.rf_freq           = TELEMETRY_RF_FREQ,
		.transmission_type = TELEMETRY_CHAN_ID_TRANS_TYPE,
		.device_type       = TELEMETRY_CHAN_ID_DEV_TYPE,
		.device_number     = device_number,
		.channel_period    = TELEMETRY_CHAN_PERIOD,
		.network_number    = TELEMETRY_ANT_NETWORK_NUM,
	};

	err_code = ant_channel_init(&t_channel_config);
	APP_ERROR_CHECK(err_code);

	telemetry_update();

	err_code = sd_ant_channel_open(TELEMETRY_CHANNEL);
	APP_ERROR_CHECK(err_code);
}
