# Stratux Integration – E80_ADSB_RL

## 1. Overview

E80_ADSB_RL provides external ADS-B traffic data to a Stratux system.

The receiver decodes 1090 MHz ADS-B signals and forwards valid Mode-S DF17 messages using the AVR text protocol.

Stratux is responsible for aircraft tracking, traffic management and position calculations.

The E80 receiver does not maintain its own aircraft database and does not calculate aircraft positions.

**Stratux integration source code:**

https://github.com/fraktal-sb/stratux/tree/E80-ADSB-RL

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
          38400 baud
               |
               v
       Raspberry Pi 3B
               |
          /dev/E80
               |
               v
            Stratux
               |
          dump1090
               |
               v
       Traffic information
         /         \
        v           v
    Stratux UI    XCSoar
```

## 3. Serial Interfaces

The E80_ADSB_RL project supports two separate serial connection methods for integration and testing.

### 3.1 Raspberry Pi GPIO UART

The E80 evaluation board transmits ADS-B data directly to the Raspberry Pi 3B.

Wiring:

| E80 signal | Raspberry Pi 3B |
|---|---|
| MCU_TXD (J2-7) | GPIO15 / RXD, physical pin 10 |
| GND | GND, physical pin 6 |

Firmware configuration:

```cpp
GPIOSerial.begin(38400);  // Raspberry Pi GPIO UART
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

The E80 evaluation board also provides a USB-UART interface through its onboard CH340 adapter.

This interface is intended for development, debugging and monitoring ADS-B messages on a PC.

It is independent of the Raspberry Pi GPIO UART.

The Stratux integration described here uses the GPIO UART and the dedicated Linux device name `/dev/E80`.

### 3.3 Stratux Device Identification – udev

Stratux identifies the E80 GPIO UART through a dedicated udev symbolic link.

The relevant rule is included in the repository file:

`10-stratux.rules`

The E80-specific rule is:

```udev
# E80 ADS-B receiver on GPIO UART
KERNEL=="ttyAMA0", SYMLINK+="E80"
```

This creates the symbolic device:

```text
/dev/E80 -> /dev/ttyAMA0
```

The rule gives the E80 a dedicated device name instead of requiring Stratux to access `/dev/ttyAMA0` directly.

**Important:** Linux device names are case-sensitive. The implemented name is `/dev/E80`, with uppercase E. The Stratux code and udev rule must use exactly the same spelling.

The rule does not create a second physical UART. `/dev/E80` and `/dev/ttyAMA0` refer to the same serial interface.

### 3.4 Stratux Ping ADS-B Input

The E80 is integrated into the existing Stratux Ping ADS-B serial input.

In `main/ping.go`, `initPingSerial()` checks for the E80 device:

```go
} else if _, err := os.Stat("/dev/E80"); err == nil {
    device = "/dev/E80"
    baudrate = 38400
}
```

The E80 is selected when the preceding supported Ping devices (`/dev/ping`, `/dev/softrf` and `/dev/pingusb`) are not present.

The existing `pingSerialReader()` processes AVR messages beginning with `*` and forwards them to the local dump1090 TCP interface on port 30001.

No separate aircraft tracking or position calculation is implemented in the E80 receiver.

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

Stratux receives these messages and handles the subsequent aircraft tracking and traffic processing.

## 5. Stratux Configuration and Installation

### 5.1 Requirements

- Raspberry Pi 3B running Stratux.
- E80 receiver connected to the Raspberry Pi GPIO UART.
- GPIO UART enabled and available as `/dev/ttyAMA0`.
- E80 firmware configured for 38400 baud.
- Stratux Ping ADS-B input enabled.
- The E80 udev rule installed.

### 5.2 Installing the udev Rule

The repository contains the extended Stratux udev rules file:

`10-stratux.rules`

The E80 rule must be present:

```udev
# E80 ADS-B receiver on GPIO UART
KERNEL=="ttyAMA0", SYMLINK+="E80"
```

On Stratux installations using the Overlay File System, the overlay must be unlocked before modifying system files.

Example:

```bash
sudo systemctl stop stratux
sudo overlayctl unlock

sudo cp 10-stratux.rules /etc/udev/rules.d/10-stratux.rules

sudo udevadm control --reload-rules
sudo udevadm trigger --action=add /sys/class/tty/ttyAMA0

ls -l /dev/E80

sudo sync
sudo overlayctl lock
sudo systemctl start stratux
```

The rule should create:

```text
/dev/E80 -> ttyAMA0
```

After a reboot, verify that `/dev/E80` still exists.

### 5.3 Verifying the Serial Input

Stop Stratux before manually accessing the UART, to avoid two processes trying to read the same serial port.

Example:

```bash
sudo systemctl stop stratux

stty -F /dev/E80 38400 raw -echo
cat /dev/E80
```

A successful reception should show AVR messages beginning with `*` and ending with `;`.

After testing, restart Stratux:

```bash
sudo systemctl start stratux
```

### 5.4 Stratux Operation

Enable the Ping ADS-B input in Stratux.

The E80 serial input is selected automatically by `initPingSerial()` when the other supported Ping devices are not present.

The E80 uses 38400 baud.

Stratux forwards the received 1090ES messages to dump1090 through its local TCP interface.

## 6. USB Serial Interface

The E80 receiver also supports USB serial communication for development and debugging.

USB serial output is useful for:

- Monitoring ADS-B messages on a PC.
- Checking AVR frame formatting.
- Testing the receiver independently of Stratux.
- Debugging the firmware.

The USB serial interface and GPIO UART are separate interfaces.

The Stratux integration described in this document uses the GPIO UART through `/dev/E80`. USB serial operation as a separate Stratux input is not part of this configuration.

## 7. Troubleshooting

### No AVR Messages on Raspberry Pi

Check:

- STM32 TX connected to Raspberry Pi RXD.
- Common GND.
- Correct baud rate: 38400.
- Raspberry Pi GPIO UART enabled.
- E80 receiver operational.

### `/dev/E80` Does Not Exist

Check the udev rule:

```bash
grep -n E80 /etc/udev/rules.d/10-stratux.rules
```

Test whether udev recognizes the rule:

```bash
sudo udevadm test /sys/class/tty/ttyAMA0 2>&1 | grep -i E80
```

Reload and trigger the rule:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger --action=add /sys/class/tty/ttyAMA0
ls -l /dev/E80
```

Expected result:

```text
/dev/E80 -> ttyAMA0
```

### AVR Messages Visible, but No Traffic in Stratux

Check:

1. Is the Ping ADS-B input enabled?
2. Does `/dev/E80` exist?
3. Is the E80 operating at 38400 baud?
4. Is Stratux selecting `/dev/E80`?
5. Are complete AVR frames being received?
6. Is another service accessing the same UART?
7. Is the dump1090 forwarding connection operational?

Check the Stratux log for messages indicating that the E80 device was selected and the Ping serial reader started.

### GPS Conflicts

The E80 and GPS inputs must not use the same physical serial interface simultaneously.

The E80 uses the GPIO UART represented by `/dev/ttyAMA0` and `/dev/E80`.

The GPS must therefore use a different interface if it is connected to the same Raspberry Pi.

Check the available devices:

```bash
ls -l /dev/serial*
ls -l /dev/ttyAMA*
ls -l /dev/ttyUSB*
```

Remember that `/dev/E80` is only a symbolic link to `/dev/ttyAMA0`; it does not provide an additional UART.

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
- 38400 baud serial communication.
- AVR-compatible DF17 messages.
- Stratux E80 integration branch: `E80-ADSB-RL`.

## 10. Project Repository

**Stratux integration:**

https://github.com/fraktal-sb/stratux/tree/E80-ADSB-RL

The branch contains the E80-specific udev rule and the corresponding Stratux Ping ADS-B serial input changes.

This integration is an experimental development project and is not a certified aviation system.
