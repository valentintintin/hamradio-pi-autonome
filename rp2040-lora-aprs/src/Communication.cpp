#include "Communication.h"
#include "ArduinoLog.h"
#include "utils.h"
#include "System.h"
#include "variant.h"

volatile bool Communication::hasInterrupt = false;

Communication::Communication(System *system) : system(system) {
}

bool Communication::begin() {
    if (lora.begin(RF_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, TX_OUTPUT_POWER, 8, 0, false) != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[RADIO] Init error"));
        _hasError = true;
        return false;
    }

    lora.setDio1Action(setHasInterrupt);
    lora.setDio2AsRfSwitch(true);
    lora.setRfSwitchPins(LORA_DIO4, RADIOLIB_NC);

    uint16_t state = lora.setRxBoostedGainMode(true);
    if (state != RADIOLIB_ERR_NONE) {
        _hasError = true;
    }

    state = lora.setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
    if (state != RADIOLIB_ERR_NONE) {
        _hasError = true;
    }

    state = lora.setCurrentLimit(140); // https://github.com/jgromes/RadioLib/discussions/489
    if (state != RADIOLIB_ERR_NONE) {
        _hasError = true;
    }

    state = lora.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        _hasError = true;
    }

    _hasError = false;

    return true;
}

void Communication::setHasInterrupt() {
    hasInterrupt = true;
}

void Communication::update() {
    if (hasInterrupt) {
        hasInterrupt = false;
        uint16_t irqFlags = lora.getIrqFlags();

        Log.traceln(F("[LORA] Interrupt with flags : %d"), irqFlags);

        if (irqFlags & RADIOLIB_SX126X_IRQ_RX_DONE) {
            memset(buffer, '\0', sizeof(buffer));
            size_t size = lora.getPacketLength();
            int state = lora.readData(buffer, size);
            if (state == RADIOLIB_ERR_NONE && size >= 15) {
                received(buffer, size, lora.getRSSI(), lora.getSNR());
            }
        } else {
            uint16_t state = lora.startReceive();
            if (state != RADIOLIB_ERR_NONE) {
                _hasError = true;
            }
        }
    }
}

bool Communication::send() {
    system->gpioLed.setState(HIGH);

    uint8_t size = Aprs::encode(&aprsPacketTx, bufferText);

    if (!size) {
        Log.errorln(F("[APRS] Error during string encode"));
        return false;
    } else {
        buffer[0] = '<';
        buffer[1]= 0xFF;
        buffer[2] = 0x01;

        for (uint8_t i = 0; i < size; i++) {
            buffer[i + 3] = bufferText[i];
            Log.verboseln(F("[LORA_TX] Payload[%d]=%X %c"), i + 3, buffer[i + 3], buffer[i + 3]);
        }

        Log.infoln(F("[LORA_TX] Start send %d chars : %s"), size, bufferText);

#ifndef USE_FAKE_RF
            int state = lora.transmit(buffer, size + 3);

            if (state == RADIOLIB_ERR_NONE) {
                sent();
            } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
                Log.errorln(F("[LORA] TX Error too long"));
                _hasError = true;
                return false;
            } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
                Log.errorln(F("[LORA] TX Error timeout"));
                _hasError = true;
                return false;
            } else {
                sprintf_P(bufferText, PSTR("[LORA] TX Error : %d"), state);
                _hasError = true;
                Log.errorln(bufferText);
                return false;
            }
#else
            System::delayWdt(1000);
            sent();
#endif

        return true;
    }
}

bool Communication::sendMessage(const char* destination, const char* message, const char* ackToConfirm) {
    Aprs::reset(&aprsPacketTx);

    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));

    strcpy(aprsPacketTx.message.destination, destination);
    strcpy(aprsPacketTx.message.message, message);

    if (strlen(ackToConfirm) > 0) {
        strcpy(aprsPacketTx.message.ackToConfirm, ackToConfirm);
    }

    aprsPacketTx.type = Message;

    return send();
}

bool Communication::sendTelemetry() {
    Aprs::reset(&aprsPacketTx);

    bool result = false;

    if (shouldSendTelemetryParams) {
        result = sendTelemetryParams();
        shouldSendTelemetryParams = !result;
    }

    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));

    sprintf_P(aprsPacketTx.comment, PSTR("Up:%ld"), millis() / 1000);
    aprsPacketTx.telemetries.telemetrySequenceNumber = telemetrySequenceNumber++;
    aprsPacketTx.telemetries.telemetriesAnalog[0].value = system->energyThread->getVoltageBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[1].value = system->energyThread->getCurrentBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[2].value = system->energyThread->getVoltageSolar();
    aprsPacketTx.telemetries.telemetriesAnalog[3].value = system->energyThread->getCurrentSolar();
    aprsPacketTx.telemetries.telemetriesBoolean[0].value = system->hasError();

    aprsPacketTx.type = Telemetry;
    result |= send();

    return result;
}

bool Communication::sendTelemetryParams() {
    Aprs::reset(&aprsPacketTx);

    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));

    // Voltage battery between 0 and 15000mV
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[0].name, PSTR("VBat")); // 7
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[0].unit, PSTR("V"));
    aprsPacketTx.telemetries.telemetriesAnalog[0].equation.b = 0.001;

    // Current charge between 0mA and 2000mA
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[1].name, PSTR("IBat")); // 6
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[1].unit, PSTR("mA"));

    // Voltage battery between 0 and 30000mV
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[2].name, PSTR("VSol")); // 5
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[2].unit, PSTR("V"));
    aprsPacketTx.telemetries.telemetriesAnalog[2].equation.b = 0.001;

    // Current charge between 0mA and 2000mA
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[3].name, PSTR("ISol")); // 5
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[3].unit, PSTR("mA"));

    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[0].name, PSTR("Err"));

    aprsPacketTx.type = TelemetryLabel;
    bool result = send();

    aprsPacketTx.type = TelemetryUnit;
    result |= send();

    aprsPacketTx.type = TelemetryEquation;
    result |= send();

    return result;
}

bool Communication::sendPosition(const char* comment) {
    Aprs::reset(&aprsPacketTx);

    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));

    aprsPacketTx.position.symbol = APRS_SYMBOL;
    aprsPacketTx.position.overlay = APRS_SYMBOL_TABLE;
    aprsPacketTx.position.latitude = APRS_LATITUDE;
    aprsPacketTx.position.longitude = APRS_LONGITUDE;
    aprsPacketTx.position.altitudeFeet = APRS_ALTITUDE * 3.28;
    aprsPacketTx.position.altitudeInComment = false;

    aprsPacketTx.weather.useHumidity = true;
    aprsPacketTx.weather.useTemperature = true;
    aprsPacketTx.weather.usePressure = true;

    aprsPacketTx.telemetries.telemetriesAnalog[0].value = system->energyThread->getVoltageBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[1].value = system->energyThread->getCurrentBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[2].value = system->energyThread->getVoltageSolar();
    aprsPacketTx.telemetries.telemetriesAnalog[3].value = system->energyThread->getCurrentSolar();
    aprsPacketTx.telemetries.telemetriesBoolean[0].value = system->hasError();

    aprsPacketTx.position.withWeather = !system->weatherThread.hasError();

    if (APRS_TELEMETRY_IN_POSITION || !aprsPacketTx.position.withWeather) {
        aprsPacketTx.position.withTelemetry = true;
        sprintf_P(aprsPacketTx.comment, PSTR("Up:%ld"), millis() / 1000);
        aprsPacketTx.telemetries.telemetrySequenceNumber = telemetrySequenceNumber++;
    } else {
        strcpy(aprsPacketTx.comment, comment);
    }

    aprsPacketTx.type = Position;

    aprsPacketTx.weather.temperatureFahrenheit = system->weatherThread.getTemperature() * 9 / 5 + 32;
    aprsPacketTx.weather.humidity = system->weatherThread.getHumidity();
    aprsPacketTx.weather.pressure = system->weatherThread.getPressure();

    return send();
}

bool Communication::sendStatus(const char* comment) {
    Aprs::reset(&aprsPacketTx);

    strcpy_P(aprsPacketTx.path, PSTR(APRS_PATH));
    strcpy_P(aprsPacketTx.source, PSTR(APRS_CALLSIGN));
    strcpy_P(aprsPacketTx.destination, PSTR(APRS_DESTINATION));

    strcpy(aprsPacketTx.comment, comment);

    aprsPacketTx.type = Status;

    return send();
}

void Communication::sent() {
    _hasError = false;

    system->gpioLed.setState(LOW);

    Log.infoln(F("[LORA_TX] End"));

    system->watchdogSlaveLoraTx.feed();

    lora.startReceive();
}

void Communication::received(uint8_t * payload, uint16_t size, float rssi, float snr) {
    Log.traceln(F("[LORA_RX] Payload of size %d, RSSI : %F and SNR : %F"), size, rssi, snr);
    Log.infoln(F("[LORA_RX] %s"), payload);

    for (uint16_t i = 0; i < size; i++) {
        Log.verboseln(F("[LORA_RX] Payload[%d]=%X %c"), i, payload[i], payload[i]);
    }

    system->gpioLed.setState(HIGH);

    bool shouldTx = false;

    if (!Aprs::decode(reinterpret_cast<const char *>(payload + sizeof(uint8_t) * 3), &aprsPacketRx)) {
        Log.errorln(F("[APRS] Error during decode"));
    } else {
        Log.traceln(F("[APRS] Decoded from %s to %s via %s"), aprsPacketRx.source, aprsPacketRx.destination, aprsPacketRx.path);

        if (strstr_P(aprsPacketRx.message.destination, PSTR(APRS_CALLSIGN)) != nullptr) {
            Log.traceln(F("[APRS] Message for me : %s"), aprsPacketRx.content);

            if (strlen(aprsPacketRx.message.message) > 0) {
                if (strlen(aprsPacketRx.message.ackToConfirm) > 0) {
                    shouldTx = sendMessage(aprsPacketRx.source, PSTR(""), aprsPacketRx.message.ackToConfirm);
                }

                bool processCommandResult = system->command.processCommand(aprsPacketRx.message.message);

                shouldTx |= sendMessage(aprsPacketRx.source, processCommandResult ? PSTR("OK") : PSTR("KO"));
            }
        } else {
#ifdef USE_DIGIPEATER
            shouldTx = Aprs::canBeDigipeated(aprsPacketRx.path, APRS_CALLSIGN);

            Log.traceln(F("[APRS] Message should TX : %T"), shouldTx);

            if (shouldTx) {
                Log.infoln(F("[APRS] Message digipeated via %s"), aprsPacketRx.path);
                strcpy(aprsPacketTx.source, aprsPacketRx.source);
                strcpy(aprsPacketTx.path, aprsPacketRx.path);
                strcpy(aprsPacketTx.destination, aprsPacketRx.destination);
                strcpy(aprsPacketTx.content, aprsPacketRx.content);
                aprsPacketTx.type = RawContent;
                shouldTx = send();
            }
#endif
        }
    }

    if (!shouldTx) {
        system->gpioLed.setState(LOW);
    }
}