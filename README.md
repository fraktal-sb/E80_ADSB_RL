# E80_ADSB_RL

**Experimental 1090 MHz ADS-B Receiver based on the Ebyte E80 module, LR2021 and RadioLib.**

E80_ADSB_RL is an experimental ADS-B receiver designed to receive 1090 MHz Mode-S signals and forward decoded ADS-B messages to a Stratux system.

The project uses an Ebyte E80 module with an LR2021 transceiver, controlled by an STM32F103C8T6 microcontroller and the RadioLib library.

The receiver decodes ADS-B messages and outputs valid DF17 frames in AVR format, making them available to compatible applications such as Stratux.

## Features

- 1090 MHz ADS-B reception
- OOK signal reception using the LR2021
- ADS-B message decoding using RadioLib
- DF17 message filtering
- CRC generation and validation
- AVR-compatible output format
- Serial interface for Stratux integration
- USB serial output for debugging and testing

## Hardware

| Component | Description |
|---|---|
| Microcontroller | STM32F103C8T6 |
| Radio module | Ebyte E80-900MBL-02 |
| RF transceiver | LR2021 |
| RF frequency | 1090 MHz |
| Modulation | OOK |
| GPS | Not required by this receiver |

The E80 module contains an internal 32 MHz TCXO powered through DIO3 at 1.8 V.

### Pin configuration

| Signal | STM32 pin |
|---|---|
| SPI MOSI | PA7 |
| SPI MISO | PA6 |
| SPI SCK | PA5 |
| RADIO_NSS | PA4 |
| RADIO_BUSY | PA3 |
| RADIO_NRST | PB0 |
| RADIO_IRQ | PA0 |
| DebugSerial TX | PA9 |
| DebugSerial RX | PA10 |
| LED1 | PB13 |
| LED2 | PB12 |

## Software

The project is developed using the Arduino framework.

### Dependencies

- Arduino STM32 core
- [RadioLib](https://github.com/jgromes/RadioLib) – version 7.8.1

RadioLib is used as an external library and is not included in this repository.

### Configuration

Main receiver parameters:

| Parameter | Value |
|---|---|
| RX frequency | 1090.0 MHz |
| RX bitrate | 2000.0 kbps |
| RX bandwidth | 3076.0 kHz |
| Output format | AVR |
| Message type | DF17 |

## AVR Output Format

Valid DF17 messages are output in the standard AVR text format:

```text
*8D3C66B49908C72E785C1AFCC42D;
*8D3C66B49908C82E585C1AA8DA1E;
```

Each message consists of:

- `*` – start of message
- 28 hexadecimal characters – 112-bit Mode-S frame
- `;` – end of message

The output is intended to be consumed by Stratux or other software supporting AVR-format ADS-B input.

## Stratux Integration

The receiver is designed to work as an external ADS-B data source for Stratux.

The intended data path is:

```text
Ebyte E80 / LR2021
        |
        v
STM32F103C8T6
        |
        v
ADS-B decoding
        |
        v
AVR message output
        |
        v
Serial / USB serial
        |
        v
Stratux
        |
        v
Traffic display
```

Stratux handles aircraft tracking and position calculations using its own GPS data.

The E80 receiver does not maintain its own aircraft database.

Further details about the Stratux configuration and serial interface will be documented in `docs/STRATUX.md`.

### Stratux Serial Interfaces

The E80 receiver supports two separate serial output paths for transmitting decoded ADS-B messages to a Stratux system.

#### 1. Raspberry Pi GPIO UART

The E80 evaluation board provides a UART interface via its J2 connector.

| E80 connection | Raspberry Pi connection |
|---|---|
| J2-7 `MCU_TXD` | GPIO15 / RXD (physical pin 10) |
| GND | GND (physical pin 6) |

Serial configuration:

- Baud rate: **38400**
- Format: **8N1**
- Data direction: E80 → Raspberry Pi
- Firmware initialization: `GPIOSerial.begin(38400);`

This connection has been successfully tested with Stratux.

#### 2. USB-UART interface

The E80 evaluation board also provides a USB-UART interface via its CH340 adapter.

A udev rule identifies the adapter and creates the device symlink `/dev/ping`:

```udev
# Ebyte E80-900M2212S with CH340 USB-UART Adapter
SUBSYSTEM=="tty", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="ping"
```

The rule is provided in the repository as `80-e80-adsb.rules`.

The USB-UART interface has been tested separately. Its simultaneous operation with the GPIO UART has **not yet been verified**.

#### Interface status

| Interface / feature | Status |
|---|---|
| GPIO UART, 38400 baud | Successfully tested |
| USB-UART via CH340 | Successfully tested separately |
| Simultaneous GPIO UART and USB-UART output | Not yet tested |

Both interfaces carry the same AVR-format ADS-B messages:

```text
*8D3C66B49908C72E785C1AFCC42D;
```

Stratux receives and processes these messages through its ADS-B serial input. Aircraft tracking and position management remain the responsibility of Stratux.


## Installation

1. Install the Arduino IDE.
2. Install the STM32 Arduino core.
3. Install RadioLib 7.8.1.
4. Connect the E80 module to the STM32 according to the pin configuration.
5. Open `E80_ADSB_RL.ino`.
6. Select the appropriate STM32 board and upload the firmware.

## Project Status

<badge>Experimental</badge>

| Feature | Status |
|---|---|
| LR2021 initialization | Working |
| 1090 MHz OOK reception | Working |
| ADS-B decoding | Working |
| DF17 filtering | Working |
| AVR output | Working |
| USB serial output | Working |
| Stratux integration | Successfully tested |
| Further interface development | In progress |

## Repository Structure

```text
E80_ADSB_RL/
├── E80_ADSB_RL.ino
├── README.md
├── LICENSE
├── CHANGELOG.md
├── .gitignore
├── docs/
├── hardware/
├── images/
├── examples/
└── 80-e80-adsb.rules
```

## Disclaimer

This is an experimental hardware and software project.

It is intended for development, testing and educational purposes. It is not a certified aviation device and must not be used as a substitute for approved aviation equipment.

## License

License information will be added separately.

## Acknowledgements

- [RadioLib](https://github.com/jgromes/RadioLib) by Jan Gromes
- [Stratux](https://github.com/b3nn0/stratux) open-source ADS-B receiver project
- The open-source ADS-B and aviation electronics community
