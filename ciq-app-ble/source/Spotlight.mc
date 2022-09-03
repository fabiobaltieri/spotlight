// vim: syntax=c

using Toybox.BluetoothLowEnergy as Ble;

class Spotlight extends BleDevice {
	private const DEVICE_NAME = "ZSpotlight";
	protected const SERVICE = Ble.stringToUuid("0000fab0-9736-46e5-872a-8a46449faa91");
	protected const READ_CHAR = Ble.stringToUuid("0000fab1-9736-46e5-872a-8a46449faa91");
	protected const READ_DESC = Ble.cccdUuid();
	protected const WRITE_CHAR = Ble.stringToUuid("0000fab2-9736-46e5-872a-8a46449faa91");

	var mode = null;
	var level = null;
	var soc = null;
	var temp = null;
	var tte = null;
	var vbatt = null;
	var dc = null;

	protected function matchDevice(name, mfg, uuids) {
		if (name != null && name.equals(DEVICE_NAME)) {
			return true;
		}

		return false;
	}

	function setActive(value) {
		write([value & 0xff]b);
	}

	private function s8(val) {
		return (val << 24) >> 24;
	}

	protected function read(value) {
		mode = (value[0] >> 4) & 0x0f;
		level = value[0] & 0x0f;
		soc = value[1];
		temp = s8(value[2]);
		tte = value[3];
		dc = value[4];
		vbatt = value[5] + (value[6] << 8);

		/*
		System.println(level + " " + mode +
			       " tte=" + tte +
			       " soc=" + soc +
			       " vbatt=" + vbatt +
			       " temp=" + temp +
			       " dc=" + dc);
		*/
	}

	protected function reset() {
		mode = null;
		level = null;
		soc = null;
		temp = null;
		tte = null;
		vbatt = null;
		dc = null;
	}
}
