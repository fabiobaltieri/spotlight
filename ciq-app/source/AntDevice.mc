// vim: syntax=c

using Toybox.System;
using Toybox.Ant;

class AntDevice extends Ant.GenericChannel {
	const DEVICE_TYPE = 0x7b;
	const PERIOD = 16384;
	const DEV_NUMBER = 0; /* 0 for search */
	const CHANNEL = 48;
	var device_cfg;

	const REOPEN_DELAY = 20;
	var open_delay = 1;
	var opened = false;
	var searching = false;
	var data_valid = false;

	var deviceNum;
	var mode;
	var level;
	var battery;
	var temp;
	var tte;

	hidden function debug(str) {
		//System.println("[Ant] " + str);
	}

	function initialize() {
		var chan_ass = new Ant.ChannelAssignment(
				Ant.CHANNEL_TYPE_RX_NOT_TX,
				Ant.NETWORK_PUBLIC);
		GenericChannel.initialize(method(:onMessage), chan_ass);

		device_cfg = new Ant.DeviceConfig({
				:deviceNumber => DEV_NUMBER,
				:deviceType => DEVICE_TYPE,
				:transmissionType => 0,
				:messagePeriod => PERIOD,
				:radioFrequency => CHANNEL,
				:searchTimeoutLowPriority => 4, // 10 seconds
				:searchThreshold => 0,
				});
	}

	function open() {
		debug("open");
		GenericChannel.setDeviceConfig(device_cfg);
		GenericChannel.open();
		opened = true;
		searching = true;
	}

	function maybe_open() {
		if (opened) {
			return;
		}
		if (open_delay > 0) {
			open_delay--;
			return;
		}
		open_delay = REOPEN_DELAY;
		open();
	}

	function close() {
		GenericChannel.close();
	}

	hidden function payloadHex(data) {
		var out;
		out = "[";
		out += data[0].format("%02x") + " ";
		out += data[1].format("%02x") + " ";
		out += data[2].format("%02x") + " ";
		out += data[3].format("%02x") + " ";
		out += data[4].format("%02x") + " ";
		out += data[5].format("%02x") + " ";
		out += data[6].format("%02x") + " ";
		out += data[7].format("%02x");
		out += "]";
		return out;
	}

	hidden function doChResponse(id, code) {
		if (id == Ant.MSG_ID_RF_EVENT && code == Ant.MSG_CODE_EVENT_RX_FAIL) {
			debug("rx fail");
		} else if (id == Ant.MSG_ID_RF_EVENT && code == Ant.MSG_CODE_EVENT_RX_SEARCH_TIMEOUT) {
			debug("search timeout");
		} else if (id == Ant.MSG_ID_RF_EVENT && code == Ant.MSG_CODE_EVENT_RX_FAIL_GO_TO_SEARCH) {
			debug("rx fail go to search");
			searching = true;
		} else if (id == Ant.MSG_ID_RF_EVENT && code == Ant.MSG_CODE_EVENT_CHANNEL_CLOSED) {
			debug("channel closed");
			opened = false;
			data_valid = false;
		} else {
			debug("channel response, id: " + id.format("%02x") + " code: " + code.format("%02x"));
		}
	}

	hidden function u8cap(val) {
		var out = val.toNumber();
		if (out > 0xff) {
			return 0xff;
		}
		return out;
	}

	function send_back(active, speed, cadence) {
		var data = [0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff];

		data[0] = 0x10; // Page 16

		data[1] = active;
		data[2] = u8cap(speed);
		data[3] = u8cap(cadence);

		var message = new Ant.Message();
		message.setPayload(data);
		GenericChannel.sendBroadcast(message);
	}

	hidden function s8(val) {
		return (val << 24) >> 24;
	}

	hidden function doMessage(data) {
		if (data[0] != 0) {
			// Page 0
			return;
		}

		mode = data[1] & 0x0f;
		level = (data[1] >> 4) & 0x0f;
		battery = data[2];
		temp = s8(data[3]);
		tte = data[4];

		data_valid = true;

		/*
		System.println("devnum=" + deviceNum +
				" tte=" + tte +
				" soc=" + battery +
				" vbatt=" + (data[6] + (data[7] << 8)) +
				" temp=" + temp
			      );
		*/
	}

	function onMessage(msg) {
		var payload = msg.getPayload();
		var msgId = msg.messageId;

		if (msgId == Ant.MSG_ID_CHANNEL_RESPONSE_EVENT) {
			// Channel Response
			doChResponse(payload[0], payload[1]);
		} else if (msgId == Ant.MSG_ID_BROADCAST_DATA) {
			// Data
			deviceNum = msg.deviceNumber;
			debug("data, dev: " + deviceNum + ": " + payloadHex(payload));
			doMessage(payload);
			searching = false;
		} else {
			// Other...
			debug("other: " + payloadHex(payload) + " devnum: " + deviceNum + " id: " + msgId);
		}
	}
}
