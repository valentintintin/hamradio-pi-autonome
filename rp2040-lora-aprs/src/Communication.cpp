#include "Communication.h"
#include "ArduinoLog.h"
#include "utils.h"
#include "System.h"

volatile bool Communication::hasInterrupt = false;

Communication::Communication(System *system) : system(system) {
}

bool Communication::begin() {
    Log.infoln(F("[LORA] Init"));

    SPI1.setSCK(LORA_SCK);
    SPI1.setTX(LORA_MOSI);
    SPI1.setRX(LORA_MISO);
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    SPI1.begin(false);

    SettingsLoRa settings = system->settings.lora;

    if (lora.begin(settings.frequency, settings.bandwidth, settings.spreadingFactor, settings.codingRate, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, settings.outputPower, LORA_PREAMBLE_LENGTH, 0, false) != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init error"));
        _hasError = true;
        return false;
    }

    lora.setDio1Action(setHasInterrupt);
    lora.setDio2AsRfSwitch(true);
    lora.setRfSwitchPins(LORA_DIO4, RADIOLIB_NC);

    uint16_t state = lora.setRxBoostedGainMode(true);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setRxBoostedGainMode"));
        _hasError = true;
        return false;
    }

    state = lora.setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setCRC"));
        _hasError = true;
        return false;
    }

    state = lora.setCurrentLimit(140); // https://github.com/jgromes/RadioLib/discussions/489
    if (state != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Init KO setCurrentLimit"));
        _hasError = true;
        return false;
    }

    if (!startReceive()) {
        return false;
    }

    Log.infoln(F("[LORA] Init OK"));

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
            startReceive();
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

        uint8_t i = 0;
        while (i++ < 3 && isChannelActive()) {
            startReceive();
            delayWdt(1000);
        }
        if (i == 3) {
            Log.errorln(F("[LORA_TX] Can't send because too much signal on channel"));
            return false;
        }

        if (system->settings.lora.txEnabled) {
            int currentState = lora.transmit(buffer, size + 3);

            if (currentState == RADIOLIB_ERR_NONE) {
                sent();
            } else if (currentState == RADIOLIB_ERR_PACKET_TOO_LONG) {
                Log.errorln(F("[LORA] TX Error too long"));
                _hasError = true;
                return false;
            } else if (currentState == RADIOLIB_ERR_TX_TIMEOUT) {
                Log.errorln(F("[LORA] TX Error timeout"));
                _hasError = true;
                return false;
            } else {
                sprintf_P(bufferText, PSTR("[LORA] TX Error : %d"), currentState);
                _hasError = true;
                Log.errorln(bufferText);
                return false;
            }
        }
        else {
            delayWdt(1000);
            sent();
        }

        return true;
    }
}

bool Communication::sendRaw(const char *raw) {
    Aprs::reset(&aprsPacketTx);

    aprsPacketTx.type = RawContent;
    strcpy(aprsPacketTx.content, raw);

    return send();
}

bool Communication::sendMessage(const char* destination, const char* message, const char* ackToConfirm) {
    Aprs::reset(&aprsPacketTx);

    SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

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

    SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    sprintf_P(aprsPacketTx.comment, PSTR("Up:%ld"), millis() / 1000);
    aprsPacketTx.telemetries.telemetrySequenceNumber = telemetrySequenceNumber++;

    uint8_t i = 0;
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getVoltageBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getCurrentBattery();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getVoltageSolar();
    aprsPacketTx.telemetries.telemetriesAnalog[i++].value = system->energyThread->getCurrentSolar();

    i = 0;

    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->watchdogMeshtastic->isGpioOn();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->watchdogLinux->isGpioOn();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->energyThread->hasError();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->weatherThread->hasError();
    aprsPacketTx.telemetries.telemetriesBoolean[i++].value = system->ldrBoxOpenedThread->isBoxOpened();

    aprsPacketTx.type = Telemetry;
    result |= send();

    return result;
}

bool Communication::sendTelemetryParams() {
    Aprs::reset(&aprsPacketTx);

    SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    uint8_t i = 0;

    // Voltage battery between 0 and 15000mV
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("VBat")); // 7
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].unit, PSTR("V"));
    aprsPacketTx.telemetries.telemetriesAnalog[i++].equation.b = 0.001;

    // Current charge between 0mA and 2000mA
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("IBat")); // 6
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i++].unit, PSTR("mA"));

    // Voltage battery between 0 and 30000mV
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("VSol")); // 5
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].unit, PSTR("V"));
    aprsPacketTx.telemetries.telemetriesAnalog[i++].equation.b = 0.001;

    // Current charge between 0mA and 2000mA
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i].name, PSTR("ISol")); // 5
    strcpy_P(aprsPacketTx.telemetries.telemetriesAnalog[i++].unit, PSTR("mA"));

    i = 0;

    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Msh"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Lnx"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Err"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Wrr"));
    strcpy_P(aprsPacketTx.telemetries.telemetriesBoolean[i++].name, PSTR("Box"));

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

    SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    aprsPacketTx.position.symbol = settings.symbol;
    aprsPacketTx.position.overlay = settings.symbolTable;
    aprsPacketTx.position.latitude = settings.latitude;
    aprsPacketTx.position.longitude = settings.longitude;
    aprsPacketTx.position.altitudeFeet = settings.altitude * 3.28;
    aprsPacketTx.position.altitudeInComment = false;

    aprsPacketTx.type = Position;

    if (system->settings.weather.enabled && !system->weatherThread->hasError()) {
        aprsPacketTx.position.withWeather =
                aprsPacketTx.weather.useHumidity =
                        aprsPacketTx.weather.useTemperature =
                                aprsPacketTx.weather.usePressure = true;

        aprsPacketTx.weather.temperatureFahrenheit = (int16_t) (system->weatherThread->getTemperature() * 9.0 / 5.0 + 32);
        aprsPacketTx.weather.humidity = (int16_t) system->weatherThread->getHumidity();
        aprsPacketTx.weather.pressure = (int16_t) system->weatherThread->getPressure();
    }

    if (settings.telemetryInPosition && !system->energyThread->hasError()) {
        aprsPacketTx.telemetries.telemetriesAnalog[0].value = system->energyThread->getVoltageBattery();
        aprsPacketTx.telemetries.telemetriesAnalog[1].value = system->energyThread->getCurrentBattery();
        aprsPacketTx.telemetries.telemetriesAnalog[2].value = system->energyThread->getVoltageSolar();
        aprsPacketTx.telemetries.telemetriesAnalog[3].value = system->energyThread->getCurrentSolar();
        aprsPacketTx.telemetries.telemetriesBoolean[0].value = system->hasError();
    }

    if (settings.telemetryInPosition) {
        aprsPacketTx.position.withTelemetry = true;
        aprsPacketTx.telemetries.telemetrySequenceNumber = telemetrySequenceNumber++;
        sprintf_P(aprsPacketTx.comment, PSTR("Up:%ld %s"), millis() / 1000, comment);
    } else {
        strcpy(aprsPacketTx.comment, comment);
    }

    return send();
}

bool Communication::sendStatus(const char* comment) {
    Aprs::reset(&aprsPacketTx);

    SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    sprintf_P(aprsPacketTx.comment, PSTR("%s Up:%ld"), comment, millis() / 1000);

    aprsPacketTx.type = Status;

    return send();
}

bool Communication::sendItem(const char *name, const char symbol, const char symbolTable, const char* comment, bool alive) {
    Aprs::reset(&aprsPacketTx);

    SettingsAprs settings = system->settings.aprs;

    strcpy(aprsPacketTx.path, settings.path);
    strcpy(aprsPacketTx.source, settings.call);
    strcpy(aprsPacketTx.destination, settings.destination);

    aprsPacketTx.position.latitude = settings.latitude;
    aprsPacketTx.position.longitude = settings.longitude;
    aprsPacketTx.position.altitudeFeet = settings.altitude * 3.28;
    aprsPacketTx.position.altitudeInComment = false;

    aprsPacketTx.position.symbol = symbol;
    aprsPacketTx.position.overlay = symbolTable;
    aprsPacketTx.item.active = alive;
    strcpy(aprsPacketTx.item.name, name);
    strcpy(aprsPacketTx.comment, comment);

    aprsPacketTx.type = Item;

    return send();
}

void Communication::sent() {
    system->gpioLed.setState(LOW);

    Log.infoln(F("[LORA_TX] End"));

    if (system->watchdogSlaveLoraTxThread->enabled) {
        system->watchdogSlaveLoraTxThread->feed();
    }

    startReceive();
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

        system->addAprsFrameReceivedToHistory(&aprsPacketRx, snr, rssi);

        SettingsAprs settings = system->settings.aprs;

        if (strstr(aprsPacketRx.message.destination, settings.call) != nullptr) {
            Log.traceln(F("[APRS] Message for me : %s"), aprsPacketRx.message.message);

            if (strlen(aprsPacketRx.message.message) > 0) {
                if (strlen(aprsPacketRx.message.ackToConfirm) > 0) {
                    shouldTx = sendMessage(aprsPacketRx.source, PSTR(""), aprsPacketRx.message.ackToConfirm);
                }

                bool processCommandResult = system->command.processCommand(nullptr, aprsPacketRx.message.message);

                sprintf_P(bufferText, PSTR("%s: %s"), processCommandResult ? PSTR("OK") : PSTR("KO"), system->command.response);

                shouldTx |= sendMessage(aprsPacketRx.source, bufferText);
            }
        } else if (settings.digipeaterEnabled) {
            shouldTx = Aprs::canBeDigipeated(aprsPacketRx.path, settings.call);

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
        }
    }

    if (!shouldTx) {
        system->gpioLed.setState(LOW);
    }
}

bool Communication::isChannelActive() {
    Log.traceln(F("[LORA] Test channel is active"));

    lora.standby();

    auto result = lora.scanChannel();
    if (result == RADIOLIB_LORA_DETECTED) {
        Log.warningln(F("[LORA] Channel is already active"));
        return true;
    }

    if (result != RADIOLIB_CHANNEL_FREE) {
        Log.errorln(F("[LORA] Error during test channel free: %d"), result);
    } else {
        Log.traceln(F("[LORA] Channel is free"));
    }

    return false;
}

bool Communication::startReceive() {
    Log.traceln(F("[LORA] Start receive"));

    lora.standby();

    if (lora.startReceiveDutyCycleAuto(LORA_PREAMBLE_LENGTH, 8, RADIOLIB_IRQ_RX_DEFAULT_FLAGS | RADIOLIB_IRQ_PREAMBLE_DETECTED) != RADIOLIB_ERR_NONE) {
        Log.errorln(F("[LORA] Start receive KO"));
        _hasError = true;
        return false;
    }

    Log.traceln(F("[LORA] Start receive OK"));

    return true;
}
