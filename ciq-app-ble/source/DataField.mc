// vim: syntax=c

using Toybox.WatchUi;
using Toybox.System;

class DataField extends WatchUi.SimpleDataField {
	private var dev;

	private var active = false;

	private const modes = ["S", "M", "A", "R"];
	private const levels = ["-", "L", "M", "H", "!"];
	private const MODE_AUTO = 2;

	function initialize(device) {
		SimpleDataField.initialize();
		label = "ZSpotlight";
		dev = device;
	}

	private function sendBack() {
		if (dev.mode == MODE_AUTO) {
			if (active) {
				dev.setActive(1);
			} else {
				dev.setActive(0);
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
		var s_mode = modes[dev.mode];
		var s_level = levels[dev.level];
		var s_battery = dev.soc;
		var s_tte = dev.tte;

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
		dev.scan();

		if (dev.scanning) {
			return "Searching...";
		} else if (!dev.paired) {
			return "Idle";
		} else if (dev.device == null) {
			return "Disconnected";
		} else if (dev.mode == null) {
			return "No data";
		}

		sendBack();

		return mode_string();
	}
}
