# Stratux Integration – E80_ADSB_RL

## 1. Overview

E80_ADSB_RL is designed to provide external ADS-B traffic data to a Stratux system.

The receiver decodes 1090 MHz ADS-B signals and forwards valid Mode-S DF17 messages using the AVR text protocol.

Stratux is responsible for aircraft tracking, traffic management and position calculations.

The E80 receiver does not maintain its own aircraft database and does not calculate aircraft positions.

## 2. System Architecture

```text
        ADS-B 1090 MHz
               |
               v
      Ebyte E80 / LR2021
               |
               v
        STM32F103C8T6
               |
               v
       RadioLib ADS-B
           decoder
               |
               v
         DF17 filtering
               |
               v
          AVR output
               |
          GPIO UART
               |
               v
       Raspberry Pi 3B
               |
               v
            Stratux
               |
               v
       Traffic information
         /         \
        v           v
    Stratux UI    XCSoar
```

## 3. Serial Interfaces

The E80_ADSB_RL project uses two separate serial connection methods for integration and testing.

### 3.1 Raspberry Pi GPIO UART

The E80 evaluation board transmits ADS-B data directly to the Raspberry Pi 3B.

Wiring:

| E80 signal | Raspberry Pi 3B |
|---|---|
| MCU_TXD (J2-7) | GPIO15 / RXD, physical pin 10 |
| GND | GND, physical pin 6 |

The firmware configuration is:

```cpp
GPIOSerial.begin(38400);  // Raspberry GPIO UART
```

Serial parameters:

| Parameter | Value |
|---|---|
| Baud rate | 38400 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Protocol | AVR |

### 3.2 USB-UART Connection

The E80 board can also communicate through its CH340 USB-UART interface.

The project includes the udev rule:

`80-e80-adsb.rules`

```udev
SUBSYSTEM=="tty", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="ping"
```

The rule creates the symbolic device `/dev/ping` for the matching USB adapter.

This allows Stratux to identify the USB serial input independently of the Raspberry Pi GPIO UART.

The udev rule does not configure the GPIO UART or its baud rate.

### 3.3 Stratux Input

Stratux uses its Ping ADS-B serial input (`ping-adsb`) to process AVR-compatible ADS-B messages.

The serial input must use the correct device and baud rate for the selected connection.

The GPIO UART uses 38400 baud.

The USB connection must be configured according to its actual firmware output settings.

### 3.4 AVR Message Format

Example output:

```text
*8D3C66B49908C72E785C1AFCC42D;
```

Each message contains:

- `*` – start delimiter
- 28 hexadecimal characters – 112-bit Mode-S frame
- `;` – end delimiter

Stratux handles aircraft tracking, traffic management and position calculations using its own GPS data.

The E80 receiver only supplies decoded ADS-B messages.


## 4. AVR Message Format

The receiver outputs valid Mode-S DF17 messages.

Example:

```text
*8D3C66B49908C72E785C1AFCC42D;
*8D3C66B49908C82E585C1AA8DA1E;
```

Frame structure:

| Element | Description |
|---|---|
| `*` | Start delimiter |
| 28 hexadecimal characters | 112-bit Mode-S message |
| `;` | End delimiter |

Each message represents one complete 112-bit Mode-S frame.

The receiver filters for DF17 messages and calculates the required Mode-S parity before output.

## 5. Stratux Configuration

Stratux must be configured to receive AVR-compatible ADS-B data through its serial input.

The serial input must:

- Use the correct Linux serial device.
- Operate at 115200 baud.
- Receive complete AVR frames.
- Be available to the Stratux ADS-B input subsystem.

The relevant Stratux input is the Ping ADS-B serial input (`ping-adsb`).

The Linux serial device name depends on the Raspberry Pi configuration and the installed udev rules.

During development, `/dev/ttyAMA0` was used to inspect the GPIO UART data.

Example:

```bash
stty -F /dev/ttyAMA0 115200 raw -echo
cat /dev/ttyAMA0
```

A successful reception should show AVR messages beginning with `*` and ending with `;`.

**Note:** Do not assume that `/dev/ttyAMA0` is the final device name on every Stratux installation. Check the actual device mapping and avoid conflicts with the Stratux GPS input.

## 6. USB Serial Interface

The E80 receiver also supports USB serial communication for development and debugging.

USB serial output is useful for:

- Monitoring ADS-B messages on a PC.
- Checking AVR frame formatting.
- Testing the receiver independently of Stratux.
- Debugging the firmware.

The USB serial interface and GPIO UART are separate interfaces.

Simultaneous output to both interfaces is a planned development feature and should only be considered supported once verified with the corresponding firmware version.

## 7. Troubleshooting

### No AVR messages on Raspberry Pi

Check the UART wiring:

- STM32 PA9 connected to Raspberry Pi RXD.
- Common GND.
- Correct baud rate.
- Raspberry Pi UART enabled.

### AVR messages visible, but no traffic in Stratux

Check:

1. Is the correct serial device selected?
2. Is the baud rate 115200?
3. Is the Ping ADS-B input enabled?
4. Is another Stratux service using the same serial device?
5. Does the input contain complete AVR frames?

### GPS conflicts

The Raspberry Pi serial interfaces must be assigned consistently.

The E80 ADS-B input and the GPS input must not use the same serial device simultaneously.

Check the actual device assignments with:

```bash
ls -l /dev/serial*
ls -l /dev/ttyAMA*
ls -l /dev/ttyUSB*
```

## 8. Project Scope

The E80 receiver provides ADS-B messages only.

The following tasks remain the responsibility of Stratux:

- Aircraft tracking.
- Aircraft position calculations.
- Traffic database management.
- Combining ADS-B traffic with GPS information.
- GDL90 output to connected displays such as XCSoar.

## 9. Tested Hardware

- Ebyte E80 / LR2021 receiver.
- STM32F103C8T6.
- Raspberry Pi 3B running Stratux.
- GPIO UART connection.
- AVR-compatible DF17 messages.

This integration is an experimental development project and is not a certified aviation system.
