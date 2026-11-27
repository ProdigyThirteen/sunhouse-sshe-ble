# Sunhouse SSHE Bluetooth control

Notes and code for controlling a Sunhouse SSHE storage heater over Bluetooth.

The SSHE is a rebadged Glen Dimplex heater that advertises over BLE as `AL6053<Sunhouse>`. This repo
has the register map, firmware that puts the heater in Home Assistant, and a couple of small Python
scripts for poking at it from a PC.

Tested on an SSHE125. The other sizes use the same controller, so they'll probably behave the same,
but I haven't checked.

## Don't touch the firmware service

The heater has a Cypress/Infineon OTA bootloader at services
`00060000-f8ce-11e4-abf4-0002a5d5c51b` and `...c51c`. Don't write to them. They're for firmware
updates, and writing to them can drop the heater into bootloader mode and leave it unable to run
until it's reflashed. Nothing here goes near them, and nothing you build should either.

## Home Assistant

[`esphome/`](esphome/) is firmware for an M5Stack ATOM Lite (any ESP32 will do with small changes).
The board sits near the heater, holds its Bluetooth connection, and gives Home Assistant:

- a thermostat with room temperature, target and the heater's modes
- boost, Advance and Holiday (with a return date)
- the heater's own User Timer and Out All Day schedules, editable a day at a time
- core temperature, off-peak supply, charging and fault status

Setup and usage are in [esphome/README.md](esphome/README.md).

## Trying it from a PC

The heater uses passkey pairing. The 6-digit passkey is printed on the rating label (next to the
model and serial number), not shown on the display. Pair once with your normal Bluetooth tools:

```bash
bluetoothctl
# scan on   -> find the AL6053 address, then:
pair <MAC>          # type the passkey from the label
trust <MAC>
```

Only one device can be connected at a time, and a bonded heater stops advertising while something's
connected to it, so disconnect everything else (the ESPHome board included) before running a script.

```bash
pip install bleak
python3 examples/read_state.py          # room and core temperature, target, mode, boost
python3 examples/set_temperature.py 20  # set the target to 20 °C
```

The scripts find the heater by name, so you don't need to know its address.

## Registers

Characteristic UUIDs are `0000XXXX-0000-1000-8000-00805f9b34fb`. The full map is in
[docs/registers.md](docs/registers.md). The ones you'll want first:

| What | Char | Access | Encoding |
|------|------|--------|----------|
| Target temperature | `1003` | read/write | `u16` little-endian °C (`14 00` = 20) |
| Mode | `1001` | read/write | byte 1: `01` Out All Day, `03` User Timer, `04` Holiday. Byte 3 = boost hours |
| Boost and Advance | `100D` | read/write | `00` none, `01` Advance, `02` boost 1 h, `03` boost 2 h |
| Holiday days | `1004` | read/write | byte 0 = days left |
| Room and core temperature | `0008` | write, then read | write `01 00 00` (room) or `03 00 00` (core), read `[sensor, °C, tenths]` |
| Weekly timers | `1002` | write, then read | write `00 <mode> <day>`, read the day's 31-byte record |
| Open window detection | `1007` | read/write | byte 0: `01` on, `00` off |
| Clock | `0006` | read/write | century, year, month, day, hour, minute, DST |

For the multi-byte registers (mode, Holiday, open window), read the current value first and change
only the byte you care about.

## Contributing

Useful things to pin down, if you have the hardware: the other SSHE sizes, and the registers at the
bottom of [docs/registers.md](docs/registers.md) that nobody's worked out yet.

## License

MIT, see [LICENSE](LICENSE). Not affiliated with Sunhouse or Glen Dimplex; the names are only here to
say what the hardware is.
