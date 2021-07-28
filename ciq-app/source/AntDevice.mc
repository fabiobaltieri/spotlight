// vim: syntax=c

using Toybox.System;
using Toybox.Ant;

class AntDevice extends Ant.GenericChannel {
	var device_cfg;

	var open_delay = 1;
	var opened = false;
	var searching = false;

	var deviceNum;

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

	hidden function s8(val) {
		return (val << 24) >> 24;
	}

	hidden function tx(data) {
		var message = new Ant.Message();
		message.setPayload(data);
		GenericChannel.sendBroadcast(message);
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
			rx(payload);
			searching = false;
		} else {
			// Other...
			debug("other: " + payloadHex(payload) + " devnum: " + deviceNum + " id: " + msgId);
		}
	}
}
