// vim: syntax=c

using Toybox.WatchUi;
using Toybox.System;

class DataField extends WatchUi.SimpleDataField {
	hidden var ant_device;
	hidden var active = false;
	hidden var speed = 0;
	hidden var cadence = 0;
	hidden var id_displayed = false;

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
	}

	function initialize_fields() {
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
		ant_device.tx(active, speed, cadence);
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

	hidden function mode_string(device, full) {
		var mode = modes[device.mode];
		var level = levels[device.level];
		var temp = device.temp;
		var battery = device.battery;
		var tte = device.tte;
		var extra;

		if (temp == -128) {
			temp = "-";
		}

		if (tte == 0xff) {
			extra = temp;
		} else if (tte == 0xfe) {
			extra = "--";
		} else if (tte > 99) {
			extra = (tte + 30) / 60 + "h";
		} else {
			extra = tte + "m";
		}

		if (battery == 0xff) {
			battery = "-";
		} else if (battery > 99) {
			battery = 99;
		}

		if (full) {
			return mode + level + " " + battery + " " + extra;
		} else {
			return mode + level + " " + battery;
		}
	}

	hidden function log_fields() {
		if (level_field == null) {
			if (!ant_device.data_valid) {
				return;
			} else {
				initialize_fields();
			}
		}
		if (!ant_device.data_valid) {
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
			id_displayed = false;
			ant_device.maybe_open();
			return "Idle";
		} else if (ant_device.searching) {
			return "Searching...";
		} else if (!id_displayed || !ant_device.data_valid) {
			id_displayed = true;
			return "id: " + ant_device.deviceNum;
		}

		if (ant_device.mode == MODE_AUTO) {
			update_send_back(info);
			send_back();
		}

		return mode_string(ant_device, true);
	}
}
