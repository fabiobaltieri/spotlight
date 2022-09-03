// vim: syntax=c

using Toybox.BluetoothLowEnergy as Ble;

class Spotlight extends BleDevice {
	private const DEVICE_NAME = "ZSpotlight";
	protected const SERVICE = Ble.stringToUuid("0000fab0-9736-46e5-872a-8a46449faa91");
	protected const READ_CHAR = Ble.stringToUuid("0000fab1-9736-46e5-872a-8a46449faa91");
	protected const READ_DESC = Ble.cccdUuid();
	protected const WRITE_CHAR = Ble.stringToUuid("0000fab2-9736-46e5-872a-8a46449faa91");

	var status = null;

	protected function matchDevice(name, mfg, uuids) {
		if (name != null && name.equals(DEVICE_NAME)) {
			return true;
		}

		return false;
	}

	function setActive(value) {
		write([value & 0xff]b);
	}

	protected function read(value) {
		status = value;
	}
}
