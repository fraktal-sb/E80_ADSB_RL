# Stratux Integration – E80_ADSB_RL

## 1. Overview

E80_ADSB_RL provides external ADS-B traffic data to a Stratux system.

The receiver decodes 1090 MHz ADS-B signals and forwards valid Mode-S DF17 messages using the AVR text protocol.

Stratux is responsible for aircraft tracking, traffic management and position calculations.

The E80 receiver does not maintain its own aircraft database and does not calculate aircraft positions.

**Stratux integration source code:**

https://github.com/fraktal-sb/stratux/tree/E80-ADSB-RL

## 2. System Architecture

Two alternative serial interfaces can connect the E80 receiver to the Raspberry Pi:

- GPIO UART (`/dev/ttyAMA0`)
- USB-UART via the onboard CH340 adapter (`/dev/ttyUSB0`)

Both interfaces are mapped to the common device name `/dev/E80`.

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
        GPIO UART or
          USB-UART
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

The E80_ADSB_RL project supports two alternative serial connection methods. Both have been successfully tested with Stratux.

Both use the same serial configuration:

| Parameter | Value |
|---|---|
| Baud rate | 38400 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Protocol | AVR |
| Data direction | E80 receiver → Raspberry Pi |

**Important:** Use only one connection method at a time. Both udev rules create the same `/dev/E80` device link.

### 3.1 Raspberry Pi GPIO UART

The E80 evaluation board transmits ADS-B data directly to the Raspberry Pi 3B.

Wiring:

| E80 signal | Raspberry Pi 3B |
|---|---|
| MCU_TXD (J2-7) | GPIO15 / RXD, physical pin 10 |
| GND | GND, physical pin 6 |

Firmware configuration:

```cpp
GPIOSerial.begin(38400);
```

The Linux device is `/dev/ttyAMA0`.

### 3.2 USB-UART Connection (CH340)

The E80 evaluation board also provides a USB-UART interface through its onboard CH340 adapter.

Connect the USB interface of the E80 board to a USB port on the Raspberry Pi.

The CH340 adapter is identified by the following USB vendor and product IDs:

| Attribute | Value |
|---|---|
| Vendor ID | `1a86` |
| Product ID | `7523` |
| Device | CH340 USB-UART converter |

The Linux device normally appears as `/dev/ttyUSB0`.

The USB-UART interface uses the same 38400 baud, 8N1 configuration as the GPIO UART.

This connection has also been successfully tested with Stratux.

### 3.3 Stratux Device Identification – udev

Stratux uses a common symbolic link named `/dev/E80` for both connection methods.

The corresponding rules are included in the repository file:

`10-stratux.rules`

The E80-specific rules are:

```udev
# E80 ADS-B receiver on GPIO UART
KERNEL=="ttyAMA0", SYMLINK+="E80"

# E80 ADS-B receiver on USB-UART (CH340)
SUBSYSTEM=="tty", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="E80"
```

Depending on the selected connection, this creates one of the following links:

**GPIO UART:**

```text
/dev/E80 -> /dev/ttyAMA0
```

**USB-UART:**

```text
/dev/E80 -> /dev/ttyUSB0
```

The link gives the E80 a consistent device name regardless of the physical serial interface.

**Important notes:**

- Linux device names are case-sensitive. The implemented name is `/dev/E80`, with uppercase E.
- `/dev/E80` is a symbolic link, not a separate physical serial interface.
- Do not connect GPIO UART and USB-UART simultaneously. Both rules create the same link.
- The selected serial interface must not be used simultaneously by another service.

## 4. Stratux Ping ADS-B Input

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

## 5. AVR Message Format

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

## 6. Stratux Configuration and Installation

### 6.1 Requirements

- Raspberry Pi 3B running Stratux.
- E80 receiver connected using either GPIO UART or USB-UART.
- E80 firmware configured for 38400 baud.
- Stratux Ping ADS-B input enabled.
- The E80 udev rules installed.
- Stratux integration branch `E80-ADSB-RL`.

### 6.2 Installing the udev Rules

The repository contains the extended Stratux udev rules file:

`10-stratux.rules`

Both E80 rules must be present:

```udev
# E80 ADS-B receiver on GPIO UART
KERNEL=="ttyAMA0", SYMLINK+="E80"

# E80 ADS-B receiver on USB-UART (CH340)
SUBSYSTEM=="tty", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="E80"
```

On Stratux installations using the Overlay File System, the overlay must be unlocked before modifying system files.

Copy the updated rules file to the Raspberry Pi. The following example assumes the file is in the current directory:

```bash
sudo systemctl stop stratux
sudo overlayctl unlock

sudo cp 10-stratux.rules /etc/udev/rules.d/10-stratux.rules

sudo udevadm control --reload-rules
sudo udevadm trigger

ls -l /dev/E80

sudo sync
sudo overlayctl lock
sudo systemctl start stratux
```

For GPIO UART, the expected link is:

```text
/dev/E80 -> ttyAMA0
```

For USB-UART, the expected link is:

```text
/dev/E80 -> ttyUSB0
```

For USB-UART, reconnect the USB adapter if necessary so that udev applies the rule.

After a reboot, verify that `/dev/E80` still exists.

### 6.3 Verifying the Serial Input

Stop Stratux before manually accessing the serial device, to avoid two processes trying to read the same serial port.

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

### 6.4 Stratux Operation

Enable the Ping ADS-B input in Stratux.

The E80 serial input is selected automatically by `initPingSerial()` when the other supported Ping devices are not present.

The E80 uses 38400 baud for both connection methods.

Stratux forwards the received 1090ES messages to dump1090 through its local TCP interface.

## 7. USB Serial Interface

The USB-UART connection is not limited to development and debugging. It is also a tested alternative for connecting the E80 receiver directly to Stratux.

The CH340 adapter is identified by its USB vendor and product IDs (`1a86:7523`).

The udev rule creates `/dev/E80`, just as it does for the GPIO UART connection.

This allows the existing Stratux serial input implementation to work without a separate USB-specific receiver driver.

The USB-UART interface can also be used independently for:

- Monitoring ADS-B messages on a PC.
- Checking AVR frame formatting.
- Testing the receiver independently of Stratux.
- Debugging the firmware.

Only one serial connection method should be used at a time.

## 8. Troubleshooting

### No AVR Messages on Raspberry Pi

Check:

- STM32 TX connected to Raspberry Pi RXD, if using GPIO UART.
- Common GND.
- Correct baud rate: 38400.
- Raspberry Pi GPIO UART enabled, if using GPIO UART.
- USB-UART adapter connected and recognized, if using USB.
- E80 receiver operational.

### `/dev/E80` Does Not Exist

Check the udev rules:

```bash
grep -n E80 /etc/udev/rules.d/10-stratux.rules
```

For GPIO UART, test whether udev recognizes the rule:

```bash
sudo udevadm test /sys/class/tty/ttyAMA0 2>&1 | grep -i E80
```

For USB-UART, identify the adapter:

```bash
lsusb
```

The CH340 adapter should appear with:

```text
1a86:7523
```

Check the serial devices and symbolic link:

```bash
ls -l /dev/E80
ls -l /dev/ttyAMA0
ls -l /dev/ttyUSB*
```

Reload the rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

If necessary, reconnect the USB adapter.

### AVR Messages Visible, but No Traffic in Stratux

Check:

1. Is the Ping ADS-B input enabled?
2. Does `/dev/E80` exist?
3. Does `/dev/E80` point to the intended serial device?
4. Is the E80 operating at 38400 baud?
5. Is Stratux selecting `/dev/E80`?
6. Are complete AVR frames being received?
7. Is another service accessing the same serial interface?
8. Is the dump1090 forwarding connection operational?

Check the Stratux log for messages indicating that the E80 device was selected and the Ping serial reader started.

Also check that no other supported Ping device is taking precedence over `/dev/E80`.

### GPS Conflicts

The E80 and GPS inputs must not use the same physical serial interface simultaneously.

When using GPIO UART, the E80 uses `/dev/ttyAMA0`, represented by `/dev/E80`.

The GPS must therefore use a different interface if it is connected to the same Raspberry Pi.

The USB-UART connection uses `/dev/ttyUSB0` and avoids occupying the GPIO UART.

Check the available devices:

```bash
ls -l /dev/serial*
ls -l /dev/ttyAMA*
ls -l /dev/ttyUSB*
```

Remember that `/dev/E80` is only a symbolic link. It does not provide an additional UART.

### Serial Terminal Shows No Data

Stratux may already have the serial device open.

Stop Stratux before testing manually:

```bash
sudo systemctl stop stratux
```

Then monitor the selected interface:

```bash
cat /dev/E80
```

Restart Stratux after testing:

```bash
sudo systemctl start stratux
```

## 9. Project Scope

The E80 receiver provides ADS-B messages only.

The following tasks remain the responsibility of Stratux:

- Aircraft tracking.
- Aircraft position calculations.
- Traffic database management.
- Combining ADS-B traffic with GPS information.
- GDL90 output to connected displays such as XCSoar.

## 10. Tested Hardware

- Ebyte E80 / LR2021 receiver.
- STM32F103C8T6.
- Raspberry Pi 3B running Stratux.
- GPIO UART connection.
- USB-UART connection via CH340.
- 38400 baud serial communication.
- AVR-compatible DF17 messages.
- Stratux E80 integration branch: `E80-ADSB-RL`.

Both GPIO UART and USB-UART connections have been successfully tested with Stratux.

## 11. Project Repository

**Stratux integration:**

https://github.com/fraktal-sb/stratux/tree/E80-ADSB-RL

The branch contains the E80-specific udev rules and the corresponding Stratux Ping ADS-B serial input changes.

This integration is an experimental development project and is not a certified aviation system.
