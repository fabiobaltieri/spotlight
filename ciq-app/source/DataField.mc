// vim: syntax=c

using Toybox.WatchUi;
using Toybox.System;

class DataField extends WatchUi.SimpleDataField {
	hidden var ant_device;

	const modes = ["S", "M", "A", "R"];
	const levels = ["-", "L", "M", "H", "!"];

	function initialize(device) {
		SimpleDataField.initialize();
		label = "Spotlight";
		ant_device = device;
	}

	function onTimerStart() {
	}

	function onTimerStop() {
	}

	function onTimerReset() {
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
		return mode_string();
	}
}
