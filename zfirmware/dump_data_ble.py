#!/usr/bin/python3

import asyncio
import time
from bleak import BleakScanner, BleakClient

uuid_battery_level_characteristic = '00002a19-0000-1000-8000-00805f9b34fb'
spotlight_status_characteristic = '0000fab1-9736-46e5-872a-8a46449faa91'

def callback(sender, data):
    state = int.from_bytes(data[0:1], byteorder='little')
    soc = int.from_bytes(data[1:2], byteorder='little')
    temp = int.from_bytes(data[2:3], byteorder='little')
    tte = int.from_bytes(data[3:4], byteorder='little')
    dc = int.from_bytes(data[4:5], byteorder='little')
    batt_mv = int.from_bytes(data[5:7], byteorder='little')
    level = state & 0x0f;
    mode = state >> 4;
    print(f"level:{level} mmode:{mode} soc:{soc} temp:{temp} tte:{tte} dc:{dc} batt_mv:{batt_mv}")

async def main():
    dev = None
    devices = await BleakScanner.discover()
    for d in devices:
        if d.name == "ZSpotlight":
            dev = d

    if not dev:
        print("Not found")
        return;

    print(f"Found: {dev}")

    async with BleakClient(dev.address) as client:
        #battery_level = await client.read_gatt_char(uuid_battery_level_characteristic)
        #print(int.from_bytes(battery_level, byteorder='big'))
        while True:
            await client.start_notify(spotlight_status_characteristic, callback)

asyncio.run(main())
