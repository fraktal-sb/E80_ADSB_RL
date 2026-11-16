# E80_ADSB_RL

ADS-B 1090 MHz receiver based on the **Ebyte E80-900MBL-02** module with LR2021 transceiver and an STM32F103C8T6 microcontroller.

The firmware uses [RadioLib](https://github.com/jgromes/RadioLib) to receive ADS-B signals in OOK mode and outputs decoded Mode-S messages in AVR format for use with [Stratux](https://github.com/b3nn0/stratux).

## Features

- ADS-B reception at 1090 MHz using OOK modulation
- LR2021 transceiver controlled through RadioLib
- STM32F103C8T6 microcontroller
- DF17 message output in AVR format
- Serial output for connection to Stratux
- GPIO UART connection to Raspberry Pi
- USB-UART connection via CH340 adapter
- Both serial connection methods successfully tested with Stratux
- No aircraft database or traffic management in the receiver itself

## Hardware

| Component | Description |
|---|---|
| Microcontroller | STM32F103C8T6 |
| Radio module | Ebyte E80-900MBL-02 |
| RF transceiver | LR2021 |
| Radio library | RadioLib 7.8.1 |
| Operating frequency | 1090 MHz |
| Modulation | OOK |

The E80 module uses an internal 32 MHz TCXO. The TCXO is supplied through DIO3 at 1.8 V.

## Pin Configuration

The following table describes the connections between the STM32F103C8T6 and the LR2021 radio module.

| Signal | STM32F103C8T6 pin |
|---|---|
| SPI MOSI | PA7 |
| SPI MISO | PA6 |
| SPI SCK | PA5 |
| RADIO_NSS | PA4 |
| RADIO_BUSY | PA3 |
| RADIO_NRST | PB0 |
| RADIO_IRQ | PA0 |

### Serial and LED connections

| Function | STM32F103C8T6 pin |
|---|---|
| DebugSerial TX | PA9 |
| DebugSerial RX | PA10 |
| LED1 | PB13 |
| LED2 | PB12 |

The LED outputs are active LOW.

See [HARDWARE.md](HARDWARE.md) for the hardware connections and interface details.

## Software

### Dependencies

- Arduino IDE or compatible Arduino development environment
- STM32duino core for STM32F1
- [RadioLib](https://github.com/jgromes/RadioLib) version 7.8.1

RadioLib is an external dependency and is not included in this repository.

### Main receiver parameters

| Parameter | Value |
|---|---:|
| RX frequency | 1090.0 MHz |
| Bitrate | 2000.0 kbps |
| RX bandwidth | 3076.0 kHz |
| Modulation | OOK |
| Output format | AVR |
| Message filter | DF17 |

## AVR Output Format

The receiver outputs ADS-B messages in the common AVR text format:

```text
*8D3C66B49908C72E785C1AFCC42D;
*8D3C66B49908C82E585C1AA8DA1E;
*8D3C66B45887541E696BE0A23D41;
```

Each message consists of:

- `*` – start marker
- 28 hexadecimal characters – 112-bit Mode-S message (14 bytes)
- `;` – end marker

The firmware currently outputs DF17 messages. The Mode-S frame includes its CRC field.

## Stratux Integration

The E80 receiver provides ADS-B traffic messages to Stratux in AVR format.

Stratux processes the received messages and performs aircraft tracking, position calculations and traffic presentation using its own GPS data.

The E80 receiver does not maintain an aircraft database and does not calculate aircraft positions.

### Data path

```text
Ebyte E80 / LR2021
       |
       v
STM32F103C8T6
       |
       v
ADS-B reception and decoding
       |
       v
AVR message output
       |
       v
GPIO UART or USB-UART
       |
       v
Raspberry Pi / Stratux
       |
       v
dump1090 interface
       |
       v
Traffic display
```

### Related repositories

| Repository | Purpose |
|---|---|
| [E80_ADSB_RL](https://github.com/fraktal-sb/E80_ADSB_RL) | E80 receiver firmware and hardware documentation |
| [Stratux – E80-ADSB-RL branch](https://github.com/fraktal-sb/stratux/tree/E80-ADSB-RL) | Stratux modifications for receiving and processing E80 AVR messages |

The Stratux branch contains the corresponding changes required to receive E80 data through either the Raspberry Pi GPIO UART or a USB-UART adapter.

### Serial interfaces

Two alternative serial connection methods are supported and have both been successfully tested with Stratux.

Both use the same serial configuration:

- Baud rate: 38400
- Data format: 8N1
- Data direction: E80 receiver → Raspberry Pi

The Stratux integration uses the common device link:

```text
/dev/E80
```

**Important:** Only one of the two connection methods should be connected at a time. Both udev rules create the same `/dev/E80` link.

#### 1. Raspberry Pi GPIO UART

Connect the E80 board's UART output to the Raspberry Pi UART input:

| E80 board | Raspberry Pi |
|---|---|
| J2-7 `MCU_TXD` | GPIO15 / RXD, physical pin 10 |
| GND | GND, physical pin 6 |

Firmware initialization:

```cpp
GPIOSerial.begin(38400);
```

On the Raspberry Pi, `/dev/E80` points to:

```text
/dev/ttyAMA0
```

The corresponding udev rule is included in the Stratux repository in `10-stratux.rules`:

```udev
# E80 ADS-B receiver on GPIO UART
KERNEL=="ttyAMA0", SYMLINK+="E80"
```

The Stratux software detects `/dev/E80` and reads the AVR messages at 38400 baud.

This connection has been successfully tested with Stratux.

#### 2. USB-UART connection (CH340)

The E80 evaluation board provides a CH340 USB-UART adapter.

Connect the USB interface of the E80 board to a USB port on the Raspberry Pi.

The CH340 adapter is identified by the following USB vendor and product IDs:

| Attribute | Value |
|---|---|
| Vendor ID | `1a86` |
| Product ID | `7523` |
| Device | CH340 USB-UART converter |

On the Raspberry Pi, the adapter normally appears as `/dev/ttyUSB0`.

The corresponding udev rule is included in the Stratux repository in `10-stratux.rules`:

```udev
# E80 ADS-B receiver on USB-UART (CH340)
SUBSYSTEM=="tty", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="E80"
```

The rule creates the same device link:

```text
/dev/E80 -> /dev/ttyUSB0
```

Stratux reads the AVR messages through `/dev/E80` at 38400 baud, just as with the GPIO UART connection.

This connection has also been successfully tested with Stratux.

#### Important notes about the serial connections

- Use either GPIO UART or USB-UART, not both simultaneously.
- Both connection methods use the same `/dev/E80` device link.
- The udev rules are maintained in the Stratux repository, in `10-stratux.rules`.
- The Raspberry Pi must not assign another service to the selected serial interface.
- When Stratux is running, it owns the serial device. Stop Stratux before testing the port manually with a serial terminal.

For further details about the Raspberry Pi installation, see [STRATUX.md](STRATUX.md).

## Installation and Configuration

1. Install the STM32 Arduino core and RadioLib 7.8.1.
2. Open `E80_ADSB_RL.ino` in the Arduino IDE.
3. Select the appropriate STM32F103C8T6 board and upload configuration.
4. Compile and upload the firmware.
5. Connect the receiver to the Raspberry Pi using either GPIO UART or USB-UART.
6. Configure the Stratux installation as described in [STRATUX.md](STRATUX.md).
7. Use the [Stratux E80-ADSB-RL branch](https://github.com/fraktal-sb/stratux/tree/E80-ADSB-RL).

The receiver configuration and hardware pin assignments are defined in the firmware.

## Project Status

| Feature | Status |
|---|---|
| LR2021 initialization | Working |
| ADS-B 1090 MHz OOK reception | Working |
| DF17 message output | Working |
| AVR output format | Working |
| Raspberry Pi GPIO UART connection | Successfully tested |
| Stratux traffic display via GPIO UART | Successfully tested |
| Raspberry Pi USB-UART connection (CH340) | Successfully tested |
| Stratux traffic display via USB-UART | Successfully tested |
| Simultaneous GPIO UART and USB-UART operation | Not tested; use only one connection at a time |

## Repository Structure

```text
E80_ADSB_RL/
├── E80_ADSB_RL.ino
├── README.md
├── HARDWARE.md
├── STRATUX.md
├── LICENSE
└── .gitignore
```

## Disclaimer

This project is experimental and intended for development and evaluation purposes.

It is not certified aviation equipment and must not be used as a primary or safety-critical source of flight information.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [RadioLib](https://github.com/jgromes/RadioLib) by Jan Gromes
- [Stratux](https://github.com/b3nn0/stratux) open-source ADS-B receiver project
- Ebyte for the E80 module and documentation
