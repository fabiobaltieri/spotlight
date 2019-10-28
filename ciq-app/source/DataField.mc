// vim: syntax=c

using Toybox.WatchUi;
using Toybox.System;

class DataField extends WatchUi.SimpleDataField {
	hidden var ant_device;

	function initialize(device) {
		SimpleDataField.initialize();
		label = "Spotlight";
		ant_device = device;
	}

	function compute(info) {
		if (ant_device.searching) {
			return "Searching...";
		}
		return "--";
	}
}
