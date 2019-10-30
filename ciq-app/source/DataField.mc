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

	function compute(info) {
		if (ant_device.searching) {
			return "Searching...";
		}
		return modes[ant_device.mode] +
			levels[ant_device.level] + " " +
			ant_device.battery + " " +
			ant_device.temp;
	}
}
