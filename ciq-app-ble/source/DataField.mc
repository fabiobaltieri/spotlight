// vim: syntax=c

using Toybox.WatchUi;
using Toybox.System;

class DataField extends WatchUi.SimpleDataField {
	private var bleDevice;

	var mode;
	var level;
	var soc;
	var temp;
	var tte;
	var vbatt;
	var dc;
	var active = false;

	const modes = ["S", "M", "A", "R"];
	const levels = ["-", "L", "M", "H", "!"];
	const MODE_AUTO = 2;

	function initialize(device) {
		SimpleDataField.initialize();
		label = "ZSpotlight";
		bleDevice = device;
	}

	private function s8(val) {
		return (val << 24) >> 24;
	}

	function sendBack() {
		if (mode == MODE_AUTO) {
			if (active) {
				bleDevice.setActive(1);
			} else {
				bleDevice.setActive(0);
			}
		}
	}

	function onTimerStart() {
		active = true;
		sendBack();
	}

	function onTimerStop() {
		active = false;
		sendBack();
	}

	function onTimerReset() {
		active = false;
		sendBack();
	}

	private function mode_string() {
		var s_mode = modes[mode];
		var s_level = levels[level];
		var s_battery = soc;
		var s_tte = tte;

		if (s_tte == 0xff || s_tte == 0) {
			s_tte = "--";
		} else if (s_tte > 99) {
			s_tte = (s_tte + 30) / 60 + "h";
		} else {
			s_tte = s_tte + "m";
		}

		if (s_battery == 0xff) {
			s_battery = "-";
		} else if (s_battery > 99) {
			s_battery = 99;
		}

		return s_mode + s_level + " " + s_battery + " " + s_tte;
	}

	function compute(info) {
		var status = bleDevice.status;

		bleDevice.scan();

		if (bleDevice.scanning) {
			return "Searching...";
		} else if (bleDevice.device == null) {
			return "Disconnected";
		} else if (status == null) {
			return "No data";
		}

		mode = (status[0] >> 4) & 0x0f;
		level = status[0] & 0x0f;
		soc = status[1];
		temp = s8(status[2]);
		tte = status[3];
		dc = status[4];
		vbatt = status[5] + (status[6] << 8);

		/*
		System.println(level + " " + mode +
			       " tte=" + tte +
			       " soc=" + soc +
			       " vbatt=" + vbatt +
			       " temp=" + temp +
			       " dc=" + dc);
		*/

		sendBack();

		return mode_string();
	}
}
