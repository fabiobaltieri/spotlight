// vim: syntax=c

class Spotlight extends AntDevice {
	const DEVICE_TYPE = 0x7b;
	const PERIOD = 16384;
	const DEV_NUMBER = 0; /* 0 for search */
	const CHANNEL = 48;
	const REOPEN_DELAY = 20;

	var data_valid = false;
	var mode;
	var level;
	var battery;
	var temp;
	var tte;
	var dc;

	function initialize() {
		AntDevice.initialize();
	}

	function tx(active, speed, cadence) {
		var data = [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff];

		data[0] = 0x10; // Page 16

		data[1] = active;
		data[2] = u8cap(speed);
		data[3] = u8cap(cadence);

		AntDevice.tx(data);
	}

	hidden function rx(data) {
		if (data[0] != 0) {
			// Page 0
			return;
		}

		mode = data[1] & 0x0f;
		level = (data[1] >> 4) & 0x0f;
		battery = data[2];
		temp = s8(data[3]);
		tte = data[4];
		dc = data[5];

		data_valid = true;

		/*
		System.println("devnum=" + deviceNum +
				" tte=" + tte +
				" soc=" + battery +
				" vbatt=" + (data[6] + (data[7] << 8)) +
				" temp=" + temp +
				" dc=" + dc);
		*/
	}
}
