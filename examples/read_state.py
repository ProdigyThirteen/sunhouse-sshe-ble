#!/usr/bin/env python3
"""Read live state from a Sunhouse SSHE storage heater over Bluetooth LE.

Standalone (uses bleak directly, no bridge). Pair the heater once first with
bluetoothctl — enter the 6-digit PIN printed on the heater's rating label.
Run when nothing else is connected to the heater (it only allows one link).

    pip install bleak
    python3 read_state.py
"""
import asyncio
from bleak import BleakClient, BleakScanner

CHAR = lambda x: f"0000{x}-0000-1000-8000-00805f9b34fb"
MODE = {1: "Out All Day", 3: "User Timer", 4: "Holiday"}


async def main():
    dev = await BleakScanner.find_device_by_filter(
        lambda d, a: (d.name or "").startswith("AL6053"), timeout=15)
    if not dev:
        raise SystemExit("heater not found — pair it first, and check nothing else is connected")
    async with BleakClient(dev) as c:
        target = await c.read_gatt_char(CHAR("1003"))   # target temp, u16 little-endian °C
        control = await c.read_gatt_char(CHAR("1001"))  # [01][mode][00][boost_hours][00][00]
        core = await c.read_gatt_char(CHAR("0008"))     # byte 1 = core temp °C
        room = await c.read_gatt_char(CHAR("0001"))     # byte 1 = room temp ×10
        print(f"Room {room[1] / 10:.1f}°C   Core {core[1]}°C")
        print(f"Target {target[0] | (target[1] << 8)}°C   "
              f"Mode {MODE.get(control[1], control[1])}   Boost {control[3]}h")


if __name__ == "__main__":
    asyncio.run(main())
