// vim: syntax=c

using Toybox.System;
using Toybox.Ant;

class AntDevice extends Ant.GenericChannel {
	const DEVICE_TYPE = 0xfb;
	const PERIOD = 32768;
	const DEV_NUMBER = 0; /* 0 for search */
	const CHANNEL = 66;

	hidden function debug(str) {
		System.println("[Ant] " + str);
	}

	function initialize() {
		var device_cfg;
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
				:searchTimeoutLowPriority => 10,
				:searchThreshold => 0,
				});
		GenericChannel.setDeviceConfig(device_cfg);
	}

	function open() {
		GenericChannel.open();
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
			debug("sarch timeout");
		} else if (id == Ant.MSG_ID_RF_EVENT && code == Ant.MSG_CODE_EVENT_RX_FAIL_GO_TO_SEARCH) {
			debug("rx fail go to search");
		} else if (id == Ant.MSG_ID_RF_EVENT && code == Ant.MSG_CODE_EVENT_CHANNEL_CLOSED) {
			debug("channel closed");
			open();
		} else {
			debug("channel response, id: " + id.format("%02x") + " code: " + code.format("%02x"));
		}
	}

	hidden function doMessage(data) {
		// TODO: message parsing and exporting
	}

	function onMessage(msg) {
		var payload = msg.getPayload();
		var deviceNum = msg.deviceNumber;
		var msgId = msg.messageId;

		if (msgId == Ant.MSG_ID_CHANNEL_RESPONSE_EVENT) {
			// Channel Response
			doChResponse(payload[0], payload[1]);
		} else if (msgId == Ant.MSG_ID_BROADCAST_DATA) {
			// Data
			debug("data, dev: " + deviceNum + ": " + payloadHex(payload));
			doMessage(payload);
		} else {
			// Other...
			debug("other: " + payloadHex(payload) + " devnum: " + deviceNum + " id: " + msgId);
		}
	}
}
