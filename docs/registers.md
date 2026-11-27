# Register map

The heater advertises as `AL6053<Sunhouse>`. Pairing uses a 6-digit passkey printed on the rating label.

Services are `00000000-000N-1000-8000-00805f9b34fb` (written S0 to S3 below) and characteristics are
`0000XXXX-0000-1000-8000-00805f9b34fb`. Nothing notifies, so everything is polled. Multi-byte values are
little-endian.

Ask for encryption as soon as you connect. The heater refuses unencrypted reads with GATT status 15 instead
of starting pairing itself.

**Don't write to `00060000-f8ce-11e4-abf4-0002a5d5c51b` or `…c51c`.** They're the Cypress firmware
bootloader, and a stray write can leave the heater stuck until it's reflashed.

## Select, then read

Some registers hold more than one value. Write a selector, then read the register, and the answer starts with
the same selector. The selector stays where it was last set, so write it before every read.

| Register | Write | Read back |
|---|---|---|
| S1/`0008` temperatures | `[N, 00, 00]`: 1 room, 2 control board, 3 storage core | `[N, °C, tenths]` |
| S1/`0003` relays | `[N, 00]`: 1 core element, 4 boost element, 5 fan | `[N, on]`, or `00 00` if there's no such relay |
| S1/`0002` element triacs | `[N, 00, 00]`: 1 or 2 | `[N, on, 00]` |
| S0/`1002` weekly timers | `[00, mode, day]` | the day's 31-byte record (see below) |
| S3/`2001` software versions | `[N, 00, 00]`: 0 control board, 1 display, 2 radio | `[N, version, 00]` |

## Service 0: control

| Char | Size | Access | What it is |
|---|---|---|---|
| `0006` | 7 | read/write | Clock: `[20][year][month][day][hour][minute][DST]`, local time, DST `01` in summer time |
| `1001` | 6 | read/write | Control block: `[01][mode][00][boost hours][00][00]`. Mode `01` Out All Day, `02` Home All Day, `03` User Timer, `04` Holiday. Read it, change the mode byte, write it back. Writing `04` starts a one-day Holiday; `01` or `03` ends one |
| `1002` | 31 | read/write | Weekly timers, one day at a time (see below) |
| `1003` | 2 | read/write | Target temperature, °C, `u16`, whole degrees within the setpoint range |
| `1004` | 4 | read/write | Holiday: byte 0 days left, byte 2 frost temperature (°C). Change byte 0 during a Holiday to move the return date |
| `1005` | 1 | read/write | Temperature unit: `00` °C |
| `1006` | 4 | read | Setpoint range: byte 0 minimum, byte 2 maximum (°C) |
| `1007` | 9 | read/write | Open window detection: byte 0, `01` on, `00` off. Change only byte 0 |
| `100D` | 1 | read/write | Override: `00` none, `01` Advance, `02` Boost 1 hour, `03` Boost 2 hours. Write `00` to cancel. A boost also shows as hours in `1001` byte 3; Advance doesn't touch `1001` |
| `1011` | 1 | read | Fault code (table below) |
| `1016` | 2 | read/write | Setback: `[on][°C]` |
| `1023` | 2 | read | Setpoint in force, °C, `u16`. Follows `1003`, and drops to the frost temperature during a Holiday |

### Weekly timers (`1002`)

Each mode has its own week: `01` Out All Day, `02` Home All Day, `03` User Timer. Days run from `00` Monday to
`06` Sunday. A day is a 31-byte record:

```
[rw][mode][day]
[start h][start m][end h][end m][°C][00]   x 4 periods
[00][00][00][00]
```

To read a day, write `[00, mode, day]` and then read the 31 bytes; `rw` reads back as `00`. To write
one, send the whole record with `rw` set to `01`.

A day has room for four periods, in order, not overlapping, and inside the day (`23:59` stands for
midnight). The spare ones still have to be filled in. In an empty day all four have zero times and
21 °C; otherwise each spare period starts and ends at the last real period's end time, with 21 °C.

Each period also stores a temperature. Keep whatever the heater had there, or write 21.

## Service 1: live status

| Char | Size | Access | What it is |
|---|---|---|---|
| `0002` | 3 | select | Element triacs |
| `0003` | 2 | select | Relays. Relay 1 is the core element's: closed doesn't mean charging, because on a dual supply the element only gets power while the off-peak supply (`000E`) is live |
| `0007` | 1 | read | Last panel key pressed |
| `0008` | 3 | select | Temperatures: room, control board, storage core |
| `000E` | 1 | read | Off-peak supply: `01` when it's live |
| `100E` | 1 | read/write | Firmware update enable. Leave it alone |
| `1020` | 1 | read/write | Factory test mode. Leave it alone |

`0001` (`02 BE FF 9B`) and its read-only copy S3/`1029` never change, so they're not room temperature.
Use `0008`.

## Service 2: options

| Char | Size | Access | What it is |
|---|---|---|---|
| `0005` | 3 | read/write | Button sound: `[on][tone][volume]` |
| `100C` | 1 | read/write | Radio link enabled |
| `1019` | 1 | read/write | Room sensor offset: `00` none, `01` to `64` adds 0.1 to 10.0 °C, above `64` subtracts ((256 − value) / 10 °C) |
| `101C` | 16 | read/write | Charge times: 4 × `[start h][start m][end h][end m]`, all zero when unset |
| `101D` | 1 | read/write | Secondary (slave) heater |
| `101E` | 1 | read/write | Additional charge, hours |

## Service 3: product

| Char | Size | Access | What it is |
|---|---|---|---|
| `000A` | 8 | read | Element power, W: `u16` element 1, `u16` element 2. An SSHE125 reads 2760 and 0 |
| `101B` | 1 | read | Heater size code |
| `2001` | 3 | select | Software versions |
| `2004` | 20 | read | Brand: `<Sunhouse>` |
| `2005` | 6 | read | Model, for example `SSHE125;B;` |
| `200A` | 6 | read | Board part number, for example `EP4078` |
| `200D` | 8 | read | Hardware identifier, for example `SGLHS6` |

`2006`, `2007` and `2008` can't be read or written.

## Fault codes (`1011`)

| Code | Fault |
|---|---|
| 0 | None |
| 20 | Thermistor failure |
| 21 | Core thermistor failure |
| 22 | Display thermistor failure |
| 30 | Room too hot |
| 32 | Display overheat |
| 34 | Fan failure |
| 40 | Internal comms failure |
| 50 | Open window detected |
| 52 | Heating disabled |
| 53 | Clock lost (power cut) |
| 60 | Wireless comms failure |
| 62 | Charge control signal failure |
| 63 | Charge times failure |

## Not worked out yet

S1: `0009`, `000D` (changes on its own, probably another temperature), `1010`.
S2: `100A`, `100B`, `100F`, `101A`.
S3: `1021`, `1022`, `1024`, `1025`, `1050`, `2002`, `2003`, `2009`, `200B`, `200C` (a snapshot of setpoints
that doesn't follow later changes).
