#include <stdbool.h>
#include <stdint.h>
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "app_pwm.h"
#include "nrf_delay.h"
#include "nrf_drv_twi.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ant.h"

#include "ant_interface.h"
#include "ant_parameters.h"
#include "ant_channel_config.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "mic280.h"
#include "utils.h"

#define ANT_DBG(a...) NRF_LOG_INFO(a)

// Telemetry Master channel
#define TELEMETRY_CHANNEL 0
#define TELEMETRY_ANT_NETWORK_NUM 0
#define TELEMETRY_CHAN_ID_DEV_NUM 666
#define TELEMETRY_CHAN_ID_DEV_TYPE 0xfb
#define TELEMETRY_CHAN_ID_TRANS_TYPE 1
#define TELEMETRY_CHAN_PERIOD 16384
#define TELEMETRY_RF_FREQ 66

// Remote Slave channel
#define REMOTE_CHANNEL 1
#define REMOTE_ANT_NETWORK_NUM 0
#define REMOTE_CHAN_ID_DEV_NUM 0
#define REMOTE_CHAN_ID_DEV_TYPE 0xfc
#define REMOTE_CHAN_ID_TRANS_TYPE 1
#define REMOTE_CHAN_PERIOD 16384
#define REMOTE_RF_FREQ 66

// Temp sensor addresses (MIC280)
static uint8_t temps_addr[] = {0x48, 0x49, 0x4a, 0x4b};

// Running state
static uint8_t tgt_levels[] = {0, 0, 0, 0};
static uint8_t cur_levels[] = {1, 1, 1, 1};
static int8_t temps[] = {INT8_MIN, INT8_MIN, INT8_MIN, INT8_MIN};

// Module state
APP_PWM_INSTANCE(PWM1, 1);
APP_PWM_INSTANCE(PWM2, 2);
static const nrf_drv_twi_t twi = NRF_DRV_TWI_INSTANCE(0);
APP_TIMER_DEF(temp_tmr);
APP_TIMER_DEF(pwm_tmr);

// Levels
static struct level {
        uint8_t a, b, c, d;
} levels[] = {
        {  0,   0,   0,   0}, // 0 - Off
        {  0,   2,   0,   0}, // 1 - Low
        {  0,  20,  20,   0}, // 2 - Medium
        {  0,  50,  50,   0}, // 3 - Maximum
        {  0,   0, 100,   0}, // 4 - Beam
};
enum {
	LEVEL_OFF = 0,
	LEVEL_LOW = 1,
	LEVEL_MEDIUM = 2,
	LEVEL_MAXIMUM = 3,
	LEVEL_BEAM = 4,
	NUM_LEVELS = 5,
};
#define LEVEL_START LEVEL_LOW
#define LEVEL_FALLBACK LEVEL_MEDIUM

// Modes
enum {
	MODE_STANDBY = 0,
	MODE_MANUAL = 1,
	MODE_AUTO = 2,
	MODE_REMOTE = 3,
};

static struct {
	uint8_t mode;
	uint8_t level;
} state;

static void twi_init(void)
{
	ret_code_t err_code;

	const nrf_drv_twi_config_t twi_config = {
		.scl                = TWI_SCL,
		.sda                = TWI_SDA,
		.frequency          = NRF_DRV_TWI_FREQ_100K,
		.interrupt_priority = APP_IRQ_PRIORITY_HIGH,
		.clear_bus_init     = false
	};

	err_code = nrf_drv_twi_init(&twi, &twi_config, NULL, NULL);
	APP_ERROR_CHECK(err_code);

	nrf_drv_twi_enable(&twi);
}

static int8_t get_max_temp(void)
{
	int8_t out = INT8_MIN;
	uint8_t i;

	for (i = 0; i < sizeof(temps); i++)
		if (temps[i] > out)
			out = temps[i];

	return out;
}

static void ant_tx_load(void)
{
	ret_code_t err_code;
	uint8_t payload[ANT_STANDARD_DATA_PAYLOAD_SIZE];

	memset(payload, 0, ANT_STANDARD_DATA_PAYLOAD_SIZE);

	payload[0] = state.mode | (state.level << 4); // Mode + Level
	payload[1] = 100; // TODO: Battery
	payload[2] = get_max_temp(); // Temperature
	payload[3] = 100; // TODO: Power de-rate
	payload[4] = 0xff;
	payload[5] = 0xff;
	payload[6] = 0xff;
	payload[7] = 0xff;

	err_code = sd_ant_broadcast_message_tx(
			TELEMETRY_CHANNEL,
			ANT_STANDARD_DATA_PAYLOAD_SIZE,
			payload);
	APP_ERROR_CHECK(err_code);

	ant_dump_message("TX", TELEMETRY_CHANNEL, payload);
}

static void temp_timer_handler(void *p_context)
{
	uint8_t i;

	for (i = 0; i < sizeof(temps); i++)
		temps[i] = mic280_read(&twi, temps_addr[i]);

	// TODO: power derate
	ant_tx_load();
}

static void pwm_update(void)
{
	ret_code_t err_code;

	if (memcmp(tgt_levels, cur_levels, sizeof(tgt_levels)) == 0)
		return;

	err_code = app_timer_start(pwm_tmr, APP_TIMER_TICKS(20), NULL);
	APP_ERROR_CHECK(err_code);
}

static void pwm_timer_handler(void *p_context)
{
	pwm_adjust_step(&cur_levels[0], tgt_levels[0]);
	pwm_adjust_step(&cur_levels[1], tgt_levels[1]);
	pwm_adjust_step(&cur_levels[2], tgt_levels[2]);
	pwm_adjust_step(&cur_levels[3], tgt_levels[3]);

	while (app_pwm_channel_duty_set(&PWM1, 0, cur_levels[0]) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM1, 1, cur_levels[1]) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 0, cur_levels[2]) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 1, cur_levels[3]) == NRF_ERROR_BUSY);

	pwm_update();
}

static void switch_short(void)
{
	if (state.mode == MODE_STANDBY)
		return;

	if (state.mode == MODE_MANUAL && state.level == LEVEL_BEAM) {
		state.mode = MODE_AUTO;
		state.level = LEVEL_START;
	} else if (state.mode == MODE_MANUAL) {
		state.level++;
	} else if (state.mode == MODE_AUTO) {
		state.mode = MODE_MANUAL;
		state.level = LEVEL_START;
	} else if (state.mode == MODE_REMOTE) {
		state.mode = MODE_MANUAL;
		state.level = LEVEL_START;
	} else {
		NRF_LOG_INFO("I should not be here");
	}
}

static void switch_long(void)
{
	if (state.mode == MODE_STANDBY) {
		state.mode = MODE_MANUAL;
		state.level = LEVEL_START;
	} else {
		state.mode = MODE_STANDBY;
		state.level = LEVEL_OFF;
	}
}

static void bsp_evt_handler(bsp_event_t event)
{
	switch (event) {
		case BSP_EVENT_KEY_0:
			switch_short();
			break;
		case BSP_EVENT_KEY_2:
			switch_long();
			break;
		default:
			break;
	}
	NRF_LOG_INFO("state mode: %d level: %d", state.mode, state.level);
	memcpy(tgt_levels, &levels[state.level], sizeof(tgt_levels));
	pwm_update();
	ant_tx_load();
}

static void hello(void)
{
	bsp_board_led_on(1);

	while (app_pwm_channel_duty_set(&PWM1, 0, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);
	while (app_pwm_channel_duty_set(&PWM1, 0, 0) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM1, 1, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);
	while (app_pwm_channel_duty_set(&PWM1, 1, 0) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 0, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);
	while (app_pwm_channel_duty_set(&PWM2, 0, 0) == NRF_ERROR_BUSY);
	while (app_pwm_channel_duty_set(&PWM2, 1, 1) == NRF_ERROR_BUSY);
	nrf_delay_ms(100);

	pwm_update();
}

static void timer_init(void)
{
	ret_code_t err_code;

	/* Temperature update cycle */
	err_code = app_timer_create(
			&temp_tmr, APP_TIMER_MODE_REPEATED, temp_timer_handler);
	APP_ERROR_CHECK(err_code);

	err_code = app_timer_start(temp_tmr, APP_TIMER_TICKS(1000), NULL);
	APP_ERROR_CHECK(err_code);

	/* PWM smoothing */
	err_code = app_timer_create(
			&pwm_tmr, APP_TIMER_MODE_SINGLE_SHOT, pwm_timer_handler);
	APP_ERROR_CHECK(err_code);
}

static void remote_process(uint8_t *payload)
{
	ant_dump_message("RX", REMOTE_CHANNEL, payload);
}

static void pwm_setup(void)
{
	ret_code_t err_code;

	app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_2CH(
			20L, POWER_LED_1, POWER_LED_2);
	app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_2CH(
			20L, POWER_LED_3, POWER_LED_4);

	pwm1_cfg.pin_polarity[0] = POWER_LED_POLARITY;
	pwm1_cfg.pin_polarity[1] = POWER_LED_POLARITY;
	pwm2_cfg.pin_polarity[0] = POWER_LED_POLARITY;
	pwm2_cfg.pin_polarity[1] = POWER_LED_POLARITY;

	err_code = app_pwm_init(&PWM1, &pwm1_cfg, NULL);
	APP_ERROR_CHECK(err_code);
	err_code = app_pwm_init(&PWM2, &pwm2_cfg, NULL);
	APP_ERROR_CHECK(err_code);

	app_pwm_enable(&PWM1);
	app_pwm_enable(&PWM2);
}

static void ant_evt_telemetry(ant_evt_t *ant_evt)
{
	uint8_t channel = ant_evt->channel;

	switch (ant_evt->event) {
		case EVENT_TX:
			break;
		case EVENT_RX:
			ant_dump_message("RX",
					channel, ant_evt->message.ANT_MESSAGE_aucPayload);
			break;
		case EVENT_CHANNEL_COLLISION:
			ANT_DBG("ANT %d: channel collision", channel);
			break;
		default:
			ANT_DBG("ANT event %d %02x", ant_evt->channel, ant_evt->event);
			break;
	}
}

static void ant_evt_remote(ant_evt_t *ant_evt)
{
	ret_code_t err_code;
	uint8_t channel = ant_evt->channel;

	bsp_board_led_invert(1);

	switch (ant_evt->event) {
		case EVENT_RX:
			remote_process(ant_evt->message.ANT_MESSAGE_aucPayload);
			break;
		case EVENT_RX_SEARCH_TIMEOUT:
		case EVENT_RX_FAIL_GO_TO_SEARCH:
			break;
		case EVENT_CHANNEL_CLOSED:
			ANT_DBG("ANT %d: channel closed", channel);
			err_code = sd_ant_channel_open(channel);
			APP_ERROR_CHECK(err_code);
			break;
		default:
			ANT_DBG("ANT event %d %02x", ant_evt->channel, ant_evt->event);
			break;
	}
}

static void ant_evt_handler(ant_evt_t *ant_evt, void *context)
{
	if (ant_evt->channel == TELEMETRY_CHANNEL)
		ant_evt_telemetry(ant_evt);
	else if (ant_evt->channel == REMOTE_CHANNEL)
		ant_evt_remote(ant_evt);
	else
		ANT_DBG("ANT ?! event %d %02x", ant_evt->channel, ant_evt->event);
}

#define ANT_OBSERVER_PRIO 1
NRF_SDH_ANT_OBSERVER(m_ant_observer, ANT_OBSERVER_PRIO, ant_evt_handler, NULL);

static void ant_channel_setup(void)
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
		.device_number     = TELEMETRY_CHAN_ID_DEV_NUM,
		.channel_period    = TELEMETRY_CHAN_PERIOD,
		.network_number    = TELEMETRY_ANT_NETWORK_NUM,
	};

	err_code = ant_channel_init(&t_channel_config);
	APP_ERROR_CHECK(err_code);

	ant_tx_load();

	err_code = sd_ant_channel_open(TELEMETRY_CHANNEL);
	APP_ERROR_CHECK(err_code);

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
			0, BSP_BUTTON_ACTION_LONG_PUSH, BSP_EVENT_KEY_2);
	APP_ERROR_CHECK(err_code);

	err_code = bsp_event_to_button_action_assign(
			1, BSP_BUTTON_ACTION_LONG_PUSH, BSP_EVENT_KEY_3);
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
	softdevice_setup();
	utils_setup();
	twi_init();
	timer_init();
	pwm_setup();

	hello();

	ant_channel_setup();

	NRF_LOG_INFO("Started...");

	for (;;) {
		NRF_LOG_FLUSH();
		nrf_pwr_mgmt_run();
	}
}
