#!/usr/bin/env python3
"""Read the state of a Sunhouse SSHE storage heater over Bluetooth LE.

Pair the heater once first with bluetoothctl, using the 6-digit passkey from its rating label.
The heater only takes one connection at a time, so disconnect anything else before running this.

    pip install bleak
    python3 read_state.py
"""
import asyncio

from bleak import BleakClient, BleakScanner


def char(short):
    return f"0000{short}-0000-1000-8000-00805f9b34fb"


MODES = {1: "Out All Day", 2: "Home All Day", 3: "User Timer", 4: "Holiday"}


async def read_temperature(client, sensor):
    """S1/0008 holds three sensors: write the sensor number, then read [number, °C, tenths]."""
    await client.write_gatt_char(char("0008"), bytes([sensor, 0, 0]), response=True)
    value = await client.read_gatt_char(char("0008"))
    return value[1] + value[2] / 10


async def main():
    device = await BleakScanner.find_device_by_filter(
        lambda d, _: (d.name or "").startswith("AL6053"), timeout=15)

    if not device:
        raise SystemExit("Heater not found. Pair it first, and check nothing else is connected to it.")

    async with BleakClient(device) as client:
        room = await read_temperature(client, 1)
        core = await read_temperature(client, 3)
        target = await client.read_gatt_char(char("1003"))    # °C, u16 little-endian
        control = await client.read_gatt_char(char("1001"))   # [01][mode][00][boost hours][00][00]

        print(f"Room {room:.1f} °C, core {core:.1f} °C")
        print(f"Target {target[0] | (target[1] << 8)} °C, {MODES.get(control[1], control[1])}, "
              f"boost {control[3]} h")


if __name__ == "__main__":
    asyncio.run(main())
