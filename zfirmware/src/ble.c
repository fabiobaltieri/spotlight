#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/smp_bt.h>

#include "img_mgmt/img_mgmt.h"
#include "os_mgmt/os_mgmt.h"

LOG_MODULE_REGISTER(ble);

#include "ble.h"
#include "state.h"

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL))
};

static uint8_t sl_status[8];
static bool notify_enabled;

static void sl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	ARG_UNUSED(attr);

	notify_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("notify_enabled:  %d", notify_enabled);
}

static ssize_t read_sl_status(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &sl_status,
				 sizeof(sl_status));
}

#define BT_UUID_SL \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE( \
		0x0000fab0, 0x9736, 0x46e5, 0x872a, 0x8a46449faa91))

#define BT_UUID_SL_STATUS \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE( \
		0x0000fab1, 0x9736, 0x46e5, 0x872a, 0x8a46449faa91))

BT_GATT_SERVICE_DEFINE(sl,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_SL),
	BT_GATT_CHARACTERISTIC(BT_UUID_SL_STATUS,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_sl_status, NULL,
			       sl_status),
	BT_GATT_CCC(sl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void ble_update(void)
{
	sl_status[0] = state.mode | (state.level << 4); // Mode + Level
	sl_status[1] = state.soc; // State of charge
	sl_status[2] = state.temp; // Temperature (C)
	sl_status[3] = state.tte; // Time to empty
	sl_status[4] = state.dc; // Duty Cycle Target
	sl_status[5] = state.batt_mv & 0xff; // Battery voltage (mV)
	sl_status[6] = state.batt_mv >> 8;
	sl_status[7] = 0;

        if (notify_enabled) {
		bt_gatt_notify(NULL, &sl.attrs[1], sl_status, sizeof(sl_status));
	}

	bt_bas_set_battery_level(state.soc);
}

static int ble_setup(const struct device *arg)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	img_mgmt_register_group();
	os_mgmt_register_group();
	smp_bt_register();

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return 0;
	}

	return 0;
}

SYS_INIT(ble_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
