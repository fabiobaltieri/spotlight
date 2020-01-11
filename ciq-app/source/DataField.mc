// vim: syntax=c

using Toybox.WatchUi;
using Toybox.System;

class DataField extends WatchUi.SimpleDataField {
	hidden var ant_device;
	hidden var active = false;
	hidden var speed = 0;
	hidden var cadence = 0;

	const modes = ["S", "M", "A", "R"];
	const levels = ["-", "L", "M", "H", "!"];
	const MODE_AUTO = 2;

	const LEVEL_FIELD_ID = 0;
	const BATT_FIELD_ID = 1;
	const TEMP_FIELD_ID = 2;
	hidden var level_field;
	hidden var batt_field;
	hidden var temp_field;

	function initialize(device) {
		SimpleDataField.initialize();
		label = "Spotlight";
		ant_device = device;

		level_field = createField(
				"level",
				LEVEL_FIELD_ID,
				FitContributor.DATA_TYPE_SINT8,
				{:mesgType=>FitContributor.MESG_TYPE_RECORD, :units=>""});
		batt_field = createField(
				"battery",
				BATT_FIELD_ID,
				FitContributor.DATA_TYPE_UINT8,
				{:mesgType=>FitContributor.MESG_TYPE_RECORD, :units=>"%"});
		temp_field = createField(
				"temperature",
				TEMP_FIELD_ID,
				FitContributor.DATA_TYPE_SINT8,
				{:mesgType=>FitContributor.MESG_TYPE_RECORD, :units=>"C"});
	}

	hidden function send_back() {
		ant_device.send_back(active, speed, cadence);
	}

	function onTimerStart() {
		active = true;
		send_back();
	}

	function onTimerStop() {
		active = false;
		send_back();
	}

	function onTimerReset() {
		active = false;
		send_back();
	}

	hidden function update_send_back(info) {
		if (info.currentSpeed) {
			speed = info.currentSpeed * 3600 / 1000;
		}
		if (info.currentCadence) {
			cadence = info.currentCadence;
		}
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
		} else if (battery > 99) {
			battery = 99;
		}

		return mode + level + " " + battery + " " + temp;
	}

	hidden function log_fields() {
		if (!ant_device.opened || ant_device.searching) {
			level_field.setData(-1);
			batt_field.setData(0);
			temp_field.setData(0);
			return;
		}
		level_field.setData(ant_device.level);
		batt_field.setData(ant_device.battery);
		if (ant_device.temp != -128) {
			temp_field.setData(ant_device.temp);
		}
	}

	function compute(info) {
		log_fields();

		if (!ant_device.opened) {
			ant_device.maybe_open();
			return "Idle";
		} else if (ant_device.searching) {
			return "Searching...";
		}

		if (ant_device.mode == MODE_AUTO) {
			update_send_back(info);
			send_back();
		}

		return mode_string();
	}
}
