# Hardware – E80_ADSB_RL

## 1. Overview

The E80_ADSB_RL receiver is built around an STM32F103C8T6 microcontroller and an Ebyte E80 module containing an LR2021 RF transceiver.

The LR2021 communicates with the STM32 through SPI and additional control signals.

### Main components

| Component | Description |
|---|---|
| MCU | STM32F103C8T6 |
| RF module | Ebyte E80-900MBL-02 |
| RF transceiver | LR2021 |
| RF frequency | 1090 MHz |
| Reception mode | OOK |
| RF library | RadioLib 7.8.1 |

## 2. STM32 – LR2021 Pin Assignment

The following pin assignments are used by the firmware.

| Function | STM32F103 pin | Description |
|---|---|---|
| SPI MOSI | PA7 | SPI data to LR2021 |
| SPI MISO | PA6 | SPI data from LR2021 |
| SPI SCK | PA5 | SPI clock |
| RADIO_NSS | PA4 | SPI chip select |
| RADIO_BUSY | PA3 | Radio busy indication |
| RADIO_NRST | PB0 | Radio reset |
| RADIO_IRQ | PA0 | Radio interrupt |

### SPI interface

The LR2021 is controlled through the STM32 hardware SPI interface.

RadioLib handles the radio communication and configuration.

## 3. Serial Interfaces

The E80 evaluation board provides two separate serial interfaces relevant to the project.

### 3.1 Raspberry Pi GPIO UART

The primary UART connection to the Raspberry Pi 3B uses the MCU_TXD signal.

| E80 J2 | Raspberry Pi 3B | Function |
|---|---|---|
| J2-7 MCU_TXD | GPIO15 / RXD (physical pin 10) | ADS-B data |
| GND | GND (physical pin 6) | Common ground |

The firmware uses:

```cpp
GPIOSerial.begin(38400);  // Raspberry GPIO UART
```

Only the TX direction is required for ADS-B data transmission.

### 3.2 USB-UART Interface

The USB connection uses a CH340 USB-UART adapter.

The Linux udev rule `80-e80-adsb.rules` identifies the adapter by its USB vendor and product IDs:

- Vendor ID: `1a86`
- Product ID: `7523`

It creates the symbolic device:

```text
/dev/ping
```

The USB-UART interface can be used independently of the Raspberry Pi GPIO UART.


## 4. Status LEDs

| LED | STM32 pin | Function |
|---|---|---|
| LED1 | PB13 | Status indication |
| LED2 | PB12 | ADS-B reception indication |

The LEDs are active LOW:

- LOW: LED ON
- HIGH: LED OFF

## 5. E80 Module Characteristics

The E80 module contains an internal 32 MHz TCXO.

According to the module configuration, the TCXO is powered through DIO3 at 1.8 V.

The LR2021 is initialized and controlled using RadioLib.

## 6. Important Notes

- The pin assignments above describe the firmware configuration.
- SPI and control signals must match the actual hardware wiring.
- The UART connections to external devices require a common GND.
- The STM32 and Raspberry Pi use 3.3 V logic levels.
- Do not connect 5 V UART signals directly to the STM32 or Raspberry Pi.
