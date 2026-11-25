# Sunhouse SSHE Bluetooth control

Notes and sample code for controlling a Sunhouse SSHE storage heater over Bluetooth.

The SSHE is a rebadged Glen Dimplex heater. It advertises over BLE as
`AL6053<Sunhouse>`, but the official apps (Dimplex Control, ConfigR) wouldn't connect
to it, so I worked out the protocol by poking at the GATT characteristics and watching
what changed when I pressed buttons on the panel. This repo is what I found, plus a
couple of small Python scripts so you can try it yourself.

Tested on an SSHE125. The other sizes use the same controller, so they'll probably
behave the same, but I haven't checked.

## Don't touch the firmware service

The heater has a Cypress/Infineon OTA bootloader at services
`00060000-f8ce-11e4-abf4-0002a5d5c51b` and `...c51c`. Don't write to them. Those are
for firmware updates, and writing to them can drop the heater into bootloader mode and
leave it unable to run until it's reflashed. The scripts here stay well away from them,
and so should anything you build.

Everything else is a plain settings register you can read and write freely.

## Pairing

The heater uses passkey pairing. The 6-digit PIN is printed on the rating label (next
to the model and serial number), not shown on the display. Pair once with your normal
Bluetooth tools:

```bash
bluetoothctl
# scan on   -> find the AL6053 address, then:
pair <MAC>          # type the PIN from the label
trust <MAC>
```

Only one device can be connected at a time. A bonded heater stops advertising while
something's connected to it, so disconnect everything else before running a script.

## Try it

```bash
pip install bleak
python3 examples/read_state.py          # room/core temp, mode, target, boost
python3 examples/set_temperature.py 20  # set target to 20 C
```

The scripts find the heater by name, so you don't need to know its address.

## Registers

Characteristic UUIDs are `0000XXXX-0000-1000-8000-00805f9b34fb`. The full map, raw
dumps and the bits I haven't figured out are in [docs/registers.md](docs/registers.md).
The ones worth knowing:

| What | Char | Access | Encoding |
|------|------|--------|----------|
| Target temperature | `1003` | read/write | `u16` little-endian °C (`14 00` = 20). `1023` mirrors it |
| Mode | `1001` | read/write | byte 1: `01` Out All Day, `03` User Timer, `04` Holiday. byte 3 = boost hours |
| Boost | `100D` | write | `00` off, `02` = 1h, `03` = 2h (3h / Autoboost not tested) |
| Open-window detection | `1007` | read/write | byte 0: `01` on, `00` off |
| Core temperature | `0008` | read | byte 1 = °C |
| Room temperature | `0001` | read | byte 1 = °C ×10 (`BE` = 19.0) |
| Comfort / frost setpoints | `000A` | read | byte 0 ×0.1 = comfort, byte 1 = frost |
| Clock | `0006` | read/write | century, year, month, day, hour, min, flag |

For the multi-byte registers (mode, open-window, clock), read the current value first
and change only the byte you care about. The others hold state.

One thing that isn't there: the 7-day timer schedule. It's not exposed as any
characteristic I could find, so you can't read or edit the on/off periods over
Bluetooth. If you want scheduling, run it yourself from something that's always on and
write the mode and target on a timetable.

## Contributing

Useful things to pin down, if you have the hardware: the other SSHE sizes, the boost
codes for 3 hours and Autoboost, and the registers in `docs/registers.md` I've marked
as unknown.

## License

MIT, see [LICENSE](LICENSE). Not affiliated with Sunhouse or Glen Dimplex; the names
are only here to say what the hardware is.
