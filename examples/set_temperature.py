#!/usr/bin/env python3
"""Set the target temperature on a Sunhouse SSHE heater over Bluetooth LE.

    pip install bleak
    python3 set_temperature.py 20        # °C, range 5–30

Pair the heater first (bluetoothctl, PIN from the label). One connection at a time.
Only the plain control registers are touched — never the firmware/OTA service.
"""
import asyncio
import sys
from bleak import BleakClient, BleakScanner

CHAR = lambda x: f"0000{x}-0000-1000-8000-00805f9b34fb"


async def main(celsius):
    if not 5 <= celsius <= 30:
        raise SystemExit("pick a temperature between 5 and 30 °C")
    dev = await BleakScanner.find_device_by_filter(
        lambda d, a: (d.name or "").startswith("AL6053"), timeout=15)
    if not dev:
        raise SystemExit("heater not found — pair it first, and check nothing else is connected")
    async with BleakClient(dev) as c:
        await c.write_gatt_char(CHAR("1003"), bytes([celsius & 0xFF, 0]), response=True)
        print(f"target set to {celsius}°C")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    asyncio.run(main(int(sys.argv[1])))
