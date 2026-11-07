# E80_ADSB_RL

ADS-B 1090 MHz receiver based on the **Ebyte E80-900MBL-02** module with LR2021 transceiver and an STM32F103C8T6 microcontroller.

The firmware uses [RadioLib](https://github.com/jgromes/RadioLib) to receive ADS-B signals in OOK mode and outputs decoded Mode-S messages in AVR format for use with [Stratux](https://github.com/b3nn0/stratux).

## Features

- ADS-B reception at 1090 MHz using OOK modulation
- LR2021 transceiver controlled through RadioLib
- STM32F103C8T6 microcontroller
- DF17 message output in AVR format
- Serial output for connection to Stratux
- GPIO UART and USB-UART connection options
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

The E80 receiver provides raw ADS-B traffic messages to Stratux. Stratux performs aircraft tracking, position calculations and traffic presentation using the received messages and its own GPS data.

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
Serial / USB-UART
        |
        v
Stratux
        |
        v
Traffic display
```

### Related repositories

| Repository | Purpose |
|---|---|
| [E80_ADSB_RL](https://github.com/fraktal-sb/E80_ADSB_RL) | E80 receiver firmware and hardware documentation |
| [Stratux – E80-ADSB branch](https://github.com/fraktal-sb/stratux/tree/E80-ADSB) | Stratux modifications for receiving and processing E80 AVR messages |

The Stratux branch contains the corresponding changes required to receive the E80 data.

### Serial interfaces

Two connection methods have been tested separately.

#### 1. Raspberry Pi GPIO UART

Connect the E80 board's UART output to the Raspberry Pi UART input:

| E80 board | Raspberry Pi |
|---|---|
| J2-7 `MCU_TXD` | GPIO15 / RXD, physical pin 10 |
| GND | GND, physical pin 6 |

Serial configuration:

- Baud rate: 38400
- Data format: 8N1
- Data direction: E80 receiver → Raspberry Pi

Firmware initialization:

```cpp
GPIOSerial.begin(38400);
```

This connection has been successfully tested with Stratux.

#### 2. USB-UART connection

The E80 evaluation board provides a CH340 USB-UART adapter.

The following udev rule creates the `/dev/ping` device link:

```udev
# Ebyte E80-900M2212S with CH340 USB-UART Adapter
SUBSYSTEM=="tty", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="ping"
```

The rule is included in this repository:

[`80-e80-adsb.rules`](80-e80-adsb.rules)

The USB-UART connection has also been tested separately with Stratux.

**Note:** Simultaneous operation of the GPIO UART and USB-UART outputs has not yet been verified.

For further details, see [STRATUX.md](STRATUX.md).

## Installation and Configuration

1. Install the STM32 Arduino core and RadioLib 7.8.1.
2. Open `E80_ADSB_RL.ino` in the Arduino IDE.
3. Select the appropriate STM32F103C8T6 board and upload configuration.
4. Compile and upload the firmware.
5. Connect the receiver to Stratux using one of the documented serial interfaces.
6. Use the [Stratux E80-ADSB branch](https://github.com/fraktal-sb/stratux/tree/E80-ADSB).

The receiver configuration and hardware pin assignments are defined in the firmware.

## Project Status

| Feature | Status |
|---|---|
| LR2021 initialization | Working |
| ADS-B 1090 MHz OOK reception | Working |
| DF17 message output | Working |
| AVR output format | Working |
| Raspberry Pi GPIO UART connection | Successfully tested |
| USB-UART connection | Successfully tested separately |
| Simultaneous GPIO UART and USB-UART operation | Not yet verified |
| Stratux traffic display via GPIO UART | Successfully tested |

## Repository Structure

```text
E80_ADSB_RL/
├── E80_ADSB_RL.ino
├── 80-e80-adsb.rules
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
