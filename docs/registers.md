# Sunhouse SSHE125 (Glen Dimplex) BLE register map

Device name: `AL6053<Sunhouse>` — the Bluetooth MAC differs per unit; find yours with `bluetoothctl devices`.

Characteristic UUIDs are `0000XXXX-0000-1000-8000-00805f9b34fb`; services are `00000000-000N-1000-8000-00805f9b34fb` (N = 0..3).

**Never write to** `00060000-f8ce-11e4-abf4-0002a5d5c51b` / `…c51c` — Cypress OTA bootloader (firmware update).

Baseline: 2026-11-25 18:12, panel showing target 20 °C, room ~17.7 °C, mode User Timer.

## Confirmed

| Svc | Char | Raw | Meaning |
|---|---|---|---|
| 0 | 0006 | `14 1a 0b 19 12 0c 01` | Clock: century 20, year 26, month 11, day 25, 18:12, last byte 01 (DST flag or weekday?) |
| 3 | 2005 | `SSHE125;B;` | Model string |
| 3 | 2004 | `<Sunhouse>` | Brand |
| 3 | 200A | `EP4078` | Part/board number |
| 3 | 200D | `SGLHS6` | Hardware/firmware ID |
| 3 | 2001 | `01 44 00` | 0x44 = 68 → matches `UI:068` on the info screen (UI firmware version) |
| 0 | 1003 | `14 00` → `15 00` | **Target temperature**, °C, u16le. Panel 20 → 21 changed it (diff 2026-11-25 18:14). **Writable**: writing `16 00` (with response) read back as 22 in both 1003 and 1023, and the panel showed 22 (2026-11-25) |
| 0 | 1023 | `14 00` → `15 00` | Changed with 1003. Probably the *active* setpoint (the one in force now) vs 1003 the comfort setting; a diff during a setback period will tell them apart |

## Boost (diff 2026-11-25 18:20: Boost 1 hour turned on from the panel)

| Svc | Char | Off → 1 h → 2 h | Meaning |
|---|---|---|---|
| 0 | 1001 | `01 03 00 00 00 00` → `…00 01 00 00` → `…00 02 00 00` | **byte 3 = boost hours** (0 = off) |
| 0 | 100D | `00` → `01` → `02` → `03` | Override selector. Confirmed: `00` none, `01` Advance, `02` Boost 1 h, `03` Boost 2 h. Write the value to set it, `00` to cancel. Advance leaves 1001 untouched; boost also sets 1001 byte 3 = hours. (`04` = 3 h likely, untested.) |

## Mode — register S0000/1001 `[01][mode][00][boost_hrs][00][00]`

Byte 1 = operating mode (user-confirmed):
- `01` = **Out All Day**
- `03` = **User Timer**
- `04` = **Holiday**
- (`02` unseen — reserved, or Advance?)

These three are the only panel modes (Boost and Setback are separate states, not modes). So 1001 is the main control block: mode in byte 1, boost hours in byte 3.

Setpoint behaviour: entering Holiday dropped both 1003 and 1023 from `14 00` (20) to `0a 00` (10), and 1004 byte 0 went `00`→`01`. So 1004 byte 2 (`0a` = 10) is the Holiday/frost target, 1004 byte 0 is the "holiday active" flag, and 1003/1023 hold the setpoint currently in force.

Register 1 / 0003 went `01 00` → `01 01` with boost but stayed `01 01` after cancelling, so it isn't the boost flag. Possibly a "something changed on the panel" marker.

Cancelling (No Boost) put 1001 byte 3 and 100D back to 0. 100D is the override selector: writing `01` starts Advance (confirmed by pressing Advance on the panel — 100D went `00`→`01`, nothing else of note changed), `02`/`03` start boost.

## Open window detection (diff 2026-11-25: OW turned off)

| Svc | Char | On → off | Meaning |
|---|---|---|---|
| 0 | 1007 | `01 00 00 00 00 00 00 00 00` → `00 …` | **byte 0 = Open Window detection** (1 = on, 0 = off) |

## Live temperatures

Confirmed live 2026-11-25 against the diagnostic screen (OSD core F=44.9, room cluster C=19.5/18.7, E3=17.7):

| Svc | Char | Decode | Meaning |
|---|---|---|---|
| 1 | 0008 | byte 1 = whole °C (read 44 vs OSD 44.9); tracked 52 boost → 45 → 44 cooling | **Core temperature °C** (byte 1, floored). byte 0 = 03 status, byte 2 wobbles |
| 1 | 0001 | `02 be ff 9b` → byte 1 ÷10 = 19.0 (OSD room sensor 18.7–19.5) | **Room air temperature ×0.1 °C** (byte 1). byte 3 = another probe (15.5); `ff` byte = unused/invalid. Same 3 bytes echo in S0003/1029 |
| 3 | 200C | `14 12 0c 0a 13` = 20,18,12,10,19 | Whole-°C bundle: comfort 20, room ~18, 12, frost 10, 19 |
| 3 | 000A | `c8 0a …` → byte0 ÷10 = 20.0, byte1 = 10 | **Comfort setpoint 20.0** (byte0 ×0.1) + **frost 10** (byte1) — matches OSD row E |

To read temperatures: core = 0008 byte 1 (°C), room = 0001 byte 1 (÷10). 200C and 000A give whole-degree setpoints.

## Changes without a setting change

| Svc | Char | Values seen | Notes |
|---|---|---|---|
| 1 | 000D | `31 61`, `31 5d`…`31 61` | **Probably a temperature sensor.** Rose with boost + higher setpoint, slowly settled. As byte0.byte1 ≈ 49.3–49.97 °C (0x31 = 49 whole degrees). Does NOT match the OSD core figure (44.5), so a different/hotter sensor (element?). Confirm by logging 000D against the OSD over a few minutes |
| 1 | 0007 | `06` → `04` | Changed while the panel was being used; UI/menu state or heating demand? |

## Strong guesses (need a diff to confirm)

| Svc | Char | Raw | Guess |
|---|---|---|---|
| 0 | 1004 | `00 00 0a 00` | 10 → setback/frost temperature? (row D shows 10.0) |
| 0 | 1001 | `01 03 00 00 00 00` | mode block? (User Timer) |
| 3 | 1024 | `00 00 05 f5` | 0x05f5 = 1525 → element power (W)? |

## 7-day timer schedule — NOT exposed over BLE

Tested by changing Wed Period 1 (08:00→09:00) on the panel and diffing: **no readable characteristic changed**, and the schedule times appear in no register in any encoding (hour byte, minutes-since-midnight, quarter-hour). The `1002`/`1012`–`1015` zero blocks are not it. The only non-readable handles are the two OTA services (off-limits) and `2006`/`2007`/`2008`, which have empty properties (inert). **Conclusion: the User Timer schedule cannot be read or written over Bluetooth — panel only.**

If you want scheduling, run it from something that stays on and write target temp (1003), mode (1001) and boost (100D) on your own timetable instead of using the heater's internal timer. Keep the heater in a steady mode and push the setpoints yourself.

## Unknown

Svc 0: 1002 (31 bytes, zero — timer table?), 1005, 1006 `07 00 1a 00`, 1007, 100D, 1011, 1012–1015 (10 bytes zero each — per-day timers?), 1016 `00 07`.
Svc 1: 0001 `02 be ff 9b`, 0002 `03 00 03`, 0003 `01 00`, 0007 `06`, 0008 `03 32 07`, 0009, 000D `"1a"`, 000E, 100E, 1010 `0x47`, 1020.
Svc 2: 0005 `01 00 00`, 100A `01`, 100B, 100C `01`, 100F, 1019, 101A (9 bytes), 101C (16 bytes), 101D, 101E `02`.
Svc 3: 000A `c8 0a 00…`, 101B `03`, 1021 `02 05 07 0a`, 1022 `01`, 1025 `0x69`, 1029 `be ff 9b`, 1050 `ff d4 00`, 2002 `01 00 64 20 06 02`, 2003 `01 0a 00`, 2009 `05`, 200B `88 fa`, 200C `14 12 0c 0a 13 00`.
2006 / 2007 / 2008 have no properties (not readable).

Live temperatures (room 17.7, core 44.5 etc. on the diagnostic screen) do not show up as plain readable values yet.
