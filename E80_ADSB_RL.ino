
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// ==================================================
// USER SETTINGS
// ==================================================

#define DEBUG       0
#define AVR_OUTPUT  1

// ==================================================
// STM32F103C8T6 CONNECTIONS
// ==================================================

#define RADIO_NSS   PA4
#define RADIO_BUSY  PA3
#define RADIO_NRST  PB0
#define RADIO_IRQ   PA0

#define LED1_PIN PB13
#define LED2_PIN PB12

#define LED_ON  LOW
#define LED_OFF HIGH

// Raspberry Pi GPIO UART: PA10 = RX, PA9 = TX
HardwareSerial GPIOSerial(PA10, PA9);

// ==================================================
// ADS-B / 1090 MHz OOK
// ==================================================

#define RX_FREQUENCY  1090.0
#define RX_BITRATE    2000.0
#define RX_BANDWIDTH  3076.0

// ==================================================
// RADIOLIB
// ==================================================

LR2021 radio = new Module(
    RADIO_NSS,
    RADIO_IRQ,
    RADIO_NRST,
    RADIO_BUSY
);

ADSBClient adsb(&radio);

// ==================================================
// GLOBAL VARIABLES
// ==================================================

volatile bool receivedFlag = false;
bool radioReady = false;

ADSBFrame frame;

char hexId[RADIOLIB_ADSB_HEX_ID_LEN];
char callsign[RADIOLIB_ADSB_CALLSIGN_LEN];

ADSBAircraftCategory category;

int altitude;
float latitude;
float longitude;
bool altitudeGnss;

unsigned long led2OffTime = 0;
bool led2Active = false;

#define LED2_ON_TIME 100

// ==================================================
// RADIO INTERRUPT CALLBACK
// ==================================================

void setFlag(void)
{
    receivedFlag = true;
}

// ==================================================
// MODE-S CRC-24
// ==================================================

uint32_t calculateModeSCRC(const uint8_t* data, uint8_t length)
{
    uint32_t crc = 0;

    for (uint8_t i = 0; i < length; i++) {

        crc ^= (uint32_t)data[i] << 16;

        for (uint8_t bit = 0; bit < 8; bit++) {

            crc <<= 1;

            if (crc & 0x1000000) {
                crc ^= 0xFFF409;
            }
        }
    }

    return crc & 0xFFFFFF;
}

// ==================================================
// AVR OUTPUT
// ==================================================

void outputAVR(const uint8_t* data)
{
#if AVR_OUTPUT

    // Only DF17 (112-bit ADS-B extended squitter)
    if ((data[0] >> 3) != 17) {
        return;
    }

    uint8_t avrFrame[14];

    memcpy(avrFrame, data, 11);

    // Calculate Mode-S CRC-24
    uint32_t crc = calculateModeSCRC(data, 11);

    avrFrame[11] = (crc >> 16) & 0xFF;
    avrFrame[12] = (crc >> 8) & 0xFF;
    avrFrame[13] = crc & 0xFF;

    // AVR output to both interfaces
    for (uint8_t port = 0; port < 2; port++) {

        Stream* outputs[] = { &Serial, &GPIOSerial };

        for (Stream* out : outputs) {
            out->print('*');

            for (uint8_t i = 0; i < 14; i++) {
                if (avrFrame[i] < 0x10) {
                    out->print('0');
                }

                out->print(avrFrame[i], HEX);
            }

            out->println(';');
        }
    }
#endif
}

// ==================================================
// DEBUG: DECODED ADS-B FRAME
// ==================================================

void printDecodedFrame()
{
#if DEBUG

    int16_t state;

    Serial.println();
    Serial.println("# ---------- ADS-B ----------");

    Serial.print("# DF: ");
    Serial.println(frame.downlinkFormat);

    Serial.print("# CA: ");
    Serial.println(frame.capability);

    Serial.print("# Message type: ");
    Serial.println((int)frame.messageType);

    // ICAO address
    state = adsb.parseHexId(&frame, hexId);

    if (state == RADIOLIB_ERR_NONE) {

        Serial.print("# ICAO: ");
        Serial.println(hexId);
    }

    // Message-specific decoding
    switch (frame.messageType) {

        case ADSBMessageType::AIRCRAFT_ID:

            state = adsb.parseCallsign(
                &frame,
                callsign,
                &category
            );

            if (state == RADIOLIB_ERR_NONE) {

                Serial.print("# Callsign: ");
                Serial.println(callsign);

                Serial.print("# Category: ");
                Serial.println((int)category);
            }

            break;

        case ADSBMessageType::AIRBORNE_POS_ALT_BARO:
        case ADSBMessageType::AIRBORNE_POS_ALT_GNSS:

            state = adsb.parseAirbornePosition(
                &frame,
                &altitude,
                &latitude,
                &longitude,
                &altitudeGnss
            );

            if (state == RADIOLIB_ERR_NONE) {

                Serial.print("# Altitude: ");
                Serial.print(altitude);

                if (altitudeGnss) {
                    Serial.println(" m (GNSS)");
                } else {
                    Serial.println(" ft (barometric)");
                }

                Serial.print("# Latitude: ");
                Serial.println(latitude, 6);

                Serial.print("# Longitude: ");
                Serial.println(longitude, 6);

            } else {

                Serial.print("# Position decode error: ");
                Serial.println(state);
            }

            break;

        default:
            break;
    }

    Serial.println("# ---------------------------");

#endif
}

// ==================================================
// PROCESS RECEIVED ADS-B PACKET
// ==================================================
/*
void processReceivedPacket()
{
    uint8_t buffer[11];

    // Read received packet
    int16_t state = radio.readData(
        buffer,
        sizeof(buffer)
    );

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        if (state != RADIOLIB_ERR_CRC_MISMATCH) {
            Serial.print("# Read error: ");
            Serial.println(state);
        }
#endif

        return;
    }

    // Decode ADS-B packet
    state = adsb.decode(buffer, &frame);

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# Decode error: ");
        Serial.println(state);
#endif

        return;
    }

    // AVR output
    outputAVR(buffer);

    // Human-readable debug output
    printDecodedFrame();

    digitalWrite(LED2_PIN, LED_ON);
    led2Active = true;
    led2OffTime = millis() + 100;

}
*/

// ==================================================
// START ADS-B RECEPTION
// ==================================================

bool startADSBRx()
{
    int16_t state = radio.startReceive();

#if DEBUG
    Serial.print("# RX start status: ");
    Serial.println(state);
#endif

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.println("# RX START FAILED");
#endif

        return false;
    }

    return true;
}

// ==================================================
// RADIO INITIALIZATION
// ==================================================

bool initRadio()
{
    // Initialize SPI
    SPI.begin();

    // Route LR2021 interrupts to DIO8
    radio.irqDioNum = 8;

    // OOK configuration
    ConfigOOK_t config;

    config.frequency = RX_FREQUENCY;
    config.bitRate = RX_BITRATE;
    config.receiverBandwidth = RX_BANDWIDTH;

#if DEBUG
    Serial.println("# Initializing OOK...");
#endif

    int16_t state = radio.beginOOK(config);

#if DEBUG
    Serial.print("# OOK init status: ");
    Serial.println(state);
#endif

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.println("# OOK INIT FAILED");
#endif

        return false;
    }

#if DEBUG
    Serial.println("# OOK INIT OK");
#endif

    // Initialize ADS-B decoder
    state = adsb.begin();

#if DEBUG
    Serial.print("# ADS-B init status: ");
    Serial.println(state);
#endif

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.println("# ADS-B INIT FAILED");
#endif

        return false;
    }

    // Enable boosted receiver gain
    state = radio.setRxBoostedGainMode(7);

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# Boosted gain error: ");
        Serial.println(state);
#endif

        return false;
    }

    // Configure CRC
    state = radio.setCRC(3, 0, 0x1FFF409UL, false);

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# CRC configuration error: ");
        Serial.println(state);
#endif

        return false;
    }

    // Configure OOK detector
    state = radio.ookDetector();

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# OOK detector error: ");
        Serial.println(state);
#endif

        return false;
    }

    // Fixed packet length: 11 bytes
    state = radio.fixedPacketLengthMode(11);

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# Packet length error: ");
        Serial.println(state);
#endif

        return false;
    }

    // Set receiver gain
    state = radio.setGain(13);

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# Gain error: ");
        Serial.println(state);
#endif

        return false;
    }

    // Calculate OOK detection threshold
    float threshold = radio.getRSSI(false) + 10;

#if DEBUG
    Serial.print("# OOK threshold: ");
    Serial.println(threshold, 1);
#endif

    state = radio.setOokDetectionThreshold(threshold);

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# Threshold error: ");
        Serial.println(state);
#endif

        return false;
    }

    // Register packet received callback
    radio.setPacketReceivedAction(setFlag);

#if DEBUG
    Serial.println("# Radio configuration OK");
#endif

    return true;
}

// ==================================================
// SETUP
// ==================================================

void setup()
{
    // LEDs
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);

    digitalWrite(LED1_PIN, LED_OFF);
    digitalWrite(LED2_PIN, LED_OFF);

    Serial.begin(115200);       // USB
    GPIOSerial.begin(38400);    // Raspberry GPIO UART

    delay(1000);

#if DEBUG
    Serial.println();
    Serial.println("# E80_ADSB_RL");
    Serial.println("# LR2021 RadioLib 7.8.1");
    Serial.println("# ADS-B 1090 MHz OOK receiver");
#endif

    // Initialize radio
    if (!initRadio()) {

#if DEBUG
        Serial.println("# INITIALIZATION FAILED");
#endif

        return;
    }

    // Start reception
    if (!startADSBRx()) {
        return;
    }

    radioReady = true;

#if DEBUG
    Serial.println("# ADS-B RX ACTIVE");
#endif
}

// ==================================================
// MAIN LOOP
// ==================================================

void loop()
{
    // LED2 unabhängig vom ADS-B-Empfang ausschalten
    if (led2Active && (long)(millis() - led2OffTime) >= 0) {
        digitalWrite(LED2_PIN, LED_OFF);
        led2Active = false;
    }

    if (!receivedFlag || !radioReady) {
        return;
    }

    receivedFlag = false;

    uint8_t buffer[11];

    int16_t state = radio.readData(buffer, sizeof(buffer));

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        if (state != RADIOLIB_ERR_CRC_MISMATCH) {
            Serial.print("# Read error: ");
            Serial.println(state);
        }
#endif
        return;
    }

    state = adsb.decode(buffer, &frame);

    if (state != RADIOLIB_ERR_NONE) {

#if DEBUG
        Serial.print("# Decode error: ");
        Serial.println(state);
#endif
        return;
    }

    // AVR output
    outputAVR(buffer);

    // Human-readable debug output
    printDecodedFrame();

    digitalWrite(LED2_PIN, LED_ON);
    led2Active = true;
    led2OffTime = millis() + 100;
}