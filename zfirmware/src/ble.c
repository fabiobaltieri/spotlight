#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/smp_bt.h>

#include "img_mgmt/img_mgmt.h"
#include "os_mgmt/os_mgmt.h"

#include "ble.h"
#include "state.h"

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL))
};

void ble_update(void)
{
	bt_bas_set_battery_level(state.soc);
}

static int ble_setup(const struct device *arg)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	img_mgmt_register_group();
	os_mgmt_register_group();
	smp_bt_register();

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	return 0;
}

SYS_INIT(ble_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
