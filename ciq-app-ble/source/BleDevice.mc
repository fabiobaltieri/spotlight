// vim: syntax=c

using Toybox.System;
using Toybox.BluetoothLowEnergy as Ble;

hidden const DEVICE_NAME = "ZSpotlight";
hidden const SL_SERVICE = Ble.stringToUuid("0000fab0-9736-46e5-872a-8a46449faa91");
hidden const SL_STATUS_CHAR = Ble.stringToUuid("0000fab1-9736-46e5-872a-8a46449faa91");
hidden const SL_STATUS_DESC = Ble.cccdUuid();
hidden const SL_ACTIVE_CHAR = Ble.stringToUuid("0000fab2-9736-46e5-872a-8a46449faa91");

class BleDevice extends Ble.BleDelegate {
	var scanning = false;
	var device = null;
	var status = null;
	var scan_delay = 5;

	hidden function debug(str) {
		//System.println("[ble] " + str);
	}

	function initialize() {
		BleDelegate.initialize();
		debug("initialize");
	}

	function setActive(value) {
		var service;
		var ch;

		if (device == null) {
			debug("setActive: not connected");
			return;
		}
		debug("setActive: " + value);

		service = device.getService(SL_SERVICE);
		ch = service.getCharacteristic(SL_ACTIVE_CHAR);
		try {
			ch.requestWrite([value & 0xff]b, {:writeType => Ble.WRITE_TYPE_DEFAULT});
		} catch (ex) {
			debug("setActive: can't start char write");
		}
	}

	function onCharacteristicChanged(ch, value) {
		debug("char read " + ch.getUuid() + " " + value);
		if (ch.getUuid().equals(SL_STATUS_CHAR)) {
			self.status = value;
		}
	}

	function setStatusNotifications(value) {
		var service;
		var ch;
		var desc;

		if (device == null) {
			debug("setStatusNotifications: not connected");
			return;
		}
		debug("setStatusNotifications: " + value);

		service = device.getService(SL_SERVICE);
		ch = service.getCharacteristic(SL_STATUS_CHAR);
		desc = ch.getDescriptor(SL_STATUS_DESC);
		desc.requestWrite([value & 0x01, 0x00]b);
	}

	function onProfileRegister(uuid, status) {
		debug("registered: " + uuid + " " + status);
	}

	function registerProfiles() {
		var profile = {
			:uuid => SL_SERVICE,
			:characteristics => [{
				:uuid => SL_STATUS_CHAR,
				:descriptors => [SL_STATUS_DESC],
                        }, {
				:uuid => SL_ACTIVE_CHAR,
			}]
		};

		BluetoothLowEnergy.registerProfile(profile);
	}

	function onScanStateChange(scanState, status) {
		debug("scanstate: " + scanState + " " + status);
		if (scanState == Ble.SCAN_STATE_SCANNING) {
			scanning = true;
		} else {
			scanning = false;
		}
	}

	function onConnectedStateChanged(device, state) {
		debug("connected: " + device.getName() + " " + state);
		if (state == Ble.CONNECTION_STATE_CONNECTED) {
			self.device = device;
			setStatusNotifications(1);
		} else {
			self.device = null;
		}
	}

	private function connect(result) {
		debug("connect");
		Ble.setScanState(Ble.SCAN_STATE_OFF);
		Ble.pairDevice(result);
	}

	private function dumpUuids(iter) {
		for (var x = iter.next(); x != null; x = iter.next()) {
			debug("uuid: " + x);
		}
	}

	private function dumpMfg(iter) {
		for (var x = iter.next(); x != null; x = iter.next()) {
			debug("mfg: companyId: " + x.get(:companyId) + " data: " + x.get(:data));
		}
	}

	function onScanResults(scanResults) {
		debug("scan results");
		var appearance, name, rssi;
		var mfg, uuids, service;
		for (var result = scanResults.next(); result != null; result = scanResults.next()) {
			appearance = result.getAppearance();
			name = result.getDeviceName();
			rssi = result.getRssi();
			mfg = result.getManufacturerSpecificDataIterator();
			uuids = result.getServiceUuids();

			debug("device: appearance: " + appearance + " name: " + name + " rssi: " + rssi);
			dumpUuids(uuids);
			dumpMfg(mfg);

			if (name != null && name.equals(DEVICE_NAME)) {
				connect(result);
				return;
			}
		}
	}

	function open() {
		registerProfiles();
	}

	function scan() {
		if (scan_delay == 0) {
			return;
		}

		debug(scan_delay);

		scan_delay--;
		if (scan_delay) {
			return;
		}

		debug("scan on");
		Ble.setScanState(Ble.SCAN_STATE_SCANNING);
	}

	function close() {
		debug("close");
		if (scanning) {
			Ble.setScanState(Ble.SCAN_STATE_OFF);
		}
		if (device != null) {
			Ble.unpairDevice(device);
		}
	}
}
