// vim: syntax=c

using Toybox.WatchUi;
using Toybox.System;

class DataField extends WatchUi.SimpleDataField {
	hidden var ant_device;
	hidden var active = false;

	const modes = ["S", "M", "A", "R"];
	const levels = ["-", "L", "M", "H", "!"];
	const MODE_AUTO = 2;

	function initialize(device) {
		SimpleDataField.initialize();
		label = "Spotlight";
		ant_device = device;
	}

	function onTimerStart() {
		active = true;
	}

	function onTimerStop() {
		active = false;
	}

	function onTimerReset() {
		active = false;
	}

	hidden function send_back(info) {
		var speed = 0;
		var cadence = 0;

		if (info.currentSpeed) {
			speed = info.currentSpeed * 3600 / 1000;
		}
		if (info.currentCadence) {
			cadence = info.currentCadence;
		}

		ant_device.send_back(active, speed, cadence);
	}

	hidden function mode_string() {
		var mode = modes[ant_device.mode];
		var level = levels[ant_device.level];
		var temp = ant_device.temp;
		var battery = ant_device.battery;

		if (temp == -128) {
			temp = "-";
		}

		if (battery == 0xff) {
			battery = "-";
		} else if (battery == 0xfe) {
			battery = "5V";
		} else if (battery > 99) {
			battery = 99;
		}

		return mode + level + " " + battery + " " + temp;
	}

	function compute(info) {
		if (ant_device.searching) {
			return "Searching...";
		}
		if (ant_device.mode == MODE_AUTO) {
			send_back(info);
		}
		return mode_string();
	}
}
