#!/usr/bin/env python3
"""Set the target temperature of a Sunhouse SSHE storage heater over Bluetooth LE.

    pip install bleak
    python3 set_temperature.py 20

Pair the heater first (bluetoothctl, passkey from the label), and disconnect anything else from it.
This only writes the target temperature register.
"""
import asyncio
import sys

from bleak import BleakClient, BleakScanner


def char(short):
    return f"0000{short}-0000-1000-8000-00805f9b34fb"


async def main(celsius):
    async with BleakClient(await find_heater()) as client:
        low, _, high, _ = await client.read_gatt_char(char("1006"))   # the heater's setpoint range

        if not low <= celsius <= high:
            raise SystemExit(f"This heater takes {low} to {high} °C.")

        await client.write_gatt_char(char("1003"), bytes([celsius, 0]), response=True)
        print(f"Target set to {celsius} °C")


async def find_heater():
    device = await BleakScanner.find_device_by_filter(
        lambda d, _: (d.name or "").startswith("AL6053"), timeout=15)

    if not device:
        raise SystemExit("Heater not found. Pair it first, and check nothing else is connected to it.")

    return device


if __name__ == "__main__":
    if len(sys.argv) != 2 or not sys.argv[1].isdigit():
        raise SystemExit(__doc__)

    asyncio.run(main(int(sys.argv[1])))
