# Sunhouse SSHE in Home Assistant (ESPHome)

Firmware for an M5Stack ATOM Lite that sits within Bluetooth range of the heater, keeps the connection
open, and shows the heater in Home Assistant. No cloud, no phone app, and no PC once it's running.

```
Heater ──Bluetooth──► ATOM Lite (ESPHome) ──Wi-Fi──► Home Assistant
```

## What you need

- An M5Stack ATOM Lite and a USB-C cable that carries data. Another ESP32 works too: change `board:`
  in `sunhouse-heater.yaml`, and the button and LED pins (`GPIO39`, `GPIO27`) or remove them.
- A 2.4 GHz Wi-Fi network that reaches the heater's room.
- Home Assistant with the ESPHome integration.
- [ESPHome](https://esphome.io) on a PC, either installed (`pip install esphome`) or through its Docker
  image (`ghcr.io/esphome/esphome`).
- The heater's Bluetooth address and the 6-digit passkey from its rating label. A phone BLE scanner
  app or `bluetoothctl` will show the address; the heater advertises as `AL6053<Sunhouse>`.

## Setup

### 1. Free the heater

It only talks to one device at a time. If a phone or PC is paired with it,
remove the pairing there first.

### 2. Fill in the secrets

```bash
cd esphome
cp secrets.example.yaml secrets.yaml
```

Edit `secrets.yaml`: your Wi-Fi details, a fallback hotspot password, an API key
(`openssl rand -base64 32`), the heater's address and its passkey. Keep `secrets.yaml` out of git.

### 3. Flash the ATOM over USB

```bash
esphome run sunhouse-heater.yaml
```

Pick the serial port when it asks. The first build downloads the ESP32 toolchain and takes a few
minutes. If the upload fails at the fast baud rate, ESPHome retries at 115200 by itself, which the
ATOM is happy with. With Docker, pass the port through:

```bash
docker run --rm -v "$PWD":/config --device=/dev/ttyUSB0 ghcr.io/esphome/esphome run sunhouse-heater.yaml --device /dev/ttyUSB0
```

On Linux, a *permission denied* on the port means your user needs to be in the `dialout` group.

### 4. Put it by the heater

Any USB charger within a few metres will do. On its first connection the
heater asks for the passkey and the ATOM answers from `secrets.yaml`; after that the pairing is
remembered. The LED goes off once it's connected.

### 5. Add it to Home Assistant

It usually turns up under Settings → Devices & services as a
discovered ESPHome device. If it doesn't, add the ESPHome integration with the ATOM's IP address and
port `6053`. Either way, paste the API key from `secrets.yaml`.

### 6. Check it

The room temperature, core temperature and mode should match the heater's panel.
To watch the log over Wi-Fi:

```bash
esphome logs sunhouse-heater.yaml
```

Later updates go over Wi-Fi with the same `esphome run sunhouse-heater.yaml`.

## What you get

| Entity | What it does |
|---|---|
| Thermostat | Room temperature, target temperature (whole degrees), and the heater's modes as presets: Out All Day, User Timer, Holiday |
| Target temperature, Heater mode | The same controls on their own |
| Boost | Off, 1 hour or 2 hours. *Boost ends* shows when it finishes |
| Override | Normal, Boost 1 hour, Boost 2 hours, Advance or Holiday, as one choice. It shows what the heater is doing, and Normal cancels whatever's running |
| Quick boost | A switch for one-tap boosting from a phone or widget: on is an hour, off cancels |
| Advance | Runs the heater's output ahead of its schedule |
| Holiday, Holiday days, Holiday ends | Frost protection until a return date. Set the days, then switch Holiday on. It ends by itself; switching it off ends it early and goes back to the mode you were in |
| User Timer / Out All Day *day* | The heater's own weekly timers, one entry per day (see below) |
| Room temperature, Core temperature | Every 30 seconds, to a tenth of a degree |
| Charging, Off-peak supply | Whether the off-peak supply is live and the core element is taking it |
| Fault | The heater's fault in words, such as *Core thermistor failure (21)*. *None* when all's well |
| Open window detection | The heater's own setting, kept with the configuration entities |
| Setpoint in force, Heater connected, Wi-Fi signal, Core element relay, Restart, Status LED | Diagnostics |

### The weekly timers

Each day is a text entry with up to four heating periods:

```
7-8:30, 17-22
```

Times are `8`, `08:00` or `8:30`. Use `24` for midnight, and `off` for a day with no heating. Periods
have to be in order, can't overlap, and can't run past midnight. Press Enter and the day is written to
the heater, then read back, so the entry always shows what the heater kept. If the text can't be read,
nothing is sent and the log says why.

The timers are read again every 30 minutes, so changes made on the panel show up too.

### Apple Home

Through Home Assistant's HomeKit Bridge, the thermostat and *Override* make a tidy pair: Apple Home
shows a select as one accessory with a button per option. Add `select` to the bridge's domains, and
exclude *Heater mode*, *Boost* and the *Advance*, *Holiday* and *Quick boost* switches so they don't
turn up twice. New accessories land in the bridge's room; move them in the Home app if that's wrong.

### On the ATOM

A click on the button turns a 1-hour boost on or off, and holding it for a second starts a 2-hour one.
The LED flashes blue to confirm. Otherwise the LED is red while the ATOM isn't connected to the heater,
and off when it is.

## Troubleshooting

| What you see | Try |
|---|---|
| LED stays red, nothing about the heater in the log | Something else is still connected or paired to the heater. Remove that pairing, power-cycle the heater, or move the ATOM closer |
| The log shows the passkey request, then pairing fails | The passkey in `secrets.yaml` is wrong. Fix it, erase the ATOM's flash (`esptool erase-flash`) to clear the failed pairing, and flash it again |
| Reads fail with status 15 | The link isn't encrypted yet. It retries on every connection; if it keeps happening, re-pair as above |
| No Wi-Fi | The board can't see 5 GHz networks. Check the SSID; after a minute it opens the *Sunhouse Heater Fallback* hotspot where you can enter new details |
| Values show *Unavailable* | Check *Heater connected*. Writes are refused (and logged) while it's disconnected |

## Files

| File | What it is |
|---|---|
| `sunhouse-heater.yaml` | The device: Bluetooth link, reads, writes, button and LED |
| `timer-day.yaml` | One weekly-timer day entry, included once per day |
| `components/sunhouse_climate/` | The thermostat entity, plus the weekly-timer and fault-code helpers |
| `secrets.example.yaml` | Template for `secrets.yaml` |
