#include <stdlib.h>
#include <hardware/rtc.h>
#include "ArduinoLog.h"

#include "Command.h"
#include "System.h"
#include "utils.h"
#include "Threads/Energy/EnergyMpptChgThread.h"
#include "I2CSlave.h"
#include "Threads/Energy/EnergyIna3221Thread.h"

System* Command::system;

Command::Command(System *system) {
    Command::system = system;

    parser.registerCommand(PSTR("position"), PSTR(""), doPosition);
    parser.registerCommand(PSTR("telem"), PSTR(""), doTelemetry);
    parser.registerCommand(PSTR("telemParams"), PSTR(""), doTelemetryParams);
    parser.registerCommand(PSTR("lora"), PSTR("s"), doLora);
    parser.registerCommand(PSTR("reset"), PSTR(""), doReset);
    parser.registerCommand(PSTR("reboot"), PSTR(""), doReboot);
    parser.registerCommand(PSTR("dfu"), PSTR(""), doDfu);
    parser.registerCommand(PSTR("json"), PSTR(""), doPrintJson);
    parser.registerCommand(PSTR("ping"), PSTR(""), doPing);
    parser.registerCommand(PSTR("gpio"), PSTR("su"), doGpioOutput);
    parser.registerCommand(PSTR("set"), PSTR("ss"), doSetSetting);
    parser.registerCommand(PSTR("get"), PSTR("s"), doGetSetting);
    parser.registerCommand(PSTR("dog"), PSTR("u"), doMpptWatchdog);
    parser.registerCommand(PSTR("linuxDog"), PSTR(""), doLinuxWatchdog);
    parser.registerCommand(PSTR("time"), PSTR("u"), doSetTime);
    parser.registerCommand(PSTR("objMsh"), PSTR(""), doMeshtasticAprs);
    parser.registerCommand(PSTR("objLinux"), PSTR(""), doLinuxAprs);
    parser.registerCommand(PSTR("ldr"), PSTR(""), doGetLdrAdc);
}

bool Command::processCommand(Stream* stream, const char *command) {
    if (strlen(command) < 3) {
        Log.traceln(F("[COMMAND] Command received length %d : %s"), strlen(command), command);
        return false;
    }

    system->gpioLed.setState(true);

    Log.traceln(F("[COMMAND] Process : %s"), command);

    if (!parser.processCommand(command, response)) {
        if (stream != nullptr) {
            stream->print(F("KO "));
            stream->println(response);
        }

        Log.warningln(F("[COMMAND] %s KO (%s)"), command, response);

        ledBlink(2, 500);

        return false;
    }

    if (stream != nullptr && strlen(response)) {
        stream->println(response);
    }

    Log.infoln(F("[COMMAND] %s OK (%s)"), command, response);

    system->gpioLed.setState(false);

    return true;
}

void Command::doTelemetry(MyCommandParser::Argument *args, char *response) {
    system->sendTelemetriesThread->forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doPosition(MyCommandParser::Argument *args, char *response) {
    system->sendPositionThread->forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doTelemetryParams(MyCommandParser::Argument *args, char *response) {
    system->communication.shouldSendTelemetryParams = true;
    system->sendTelemetriesThread->forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doLora(MyCommandParser::Argument *args, char *response) {
    char *raw = args[0].asString;

    bool ok = system->communication.sendRaw(raw);

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));
}

void Command::doReset(MyCommandParser::Argument *args, char *response) {
    bool ok = system->resetSettings();

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));
}

void Command::doReboot(MyCommandParser::Argument *args, char *response) {
    system->planReboot();

    strcpy_P(response, PSTR("OK"));
}

void Command::doDfu(MyCommandParser::Argument *args, char *response) {
    system->planDfu();

    strcpy_P(response, PSTR("OK"));
}

void Command::doGpioOutput(MyCommandParser::Argument *args, char *response) {
    auto what = args[0].asString;
    auto state = args[1].asUInt64 == 1;

    GpioPin *gpio = nullptr;

    if (strcmp_P(what, PSTR("msh")) == 0) {
        gpio = system->getGpio(system->settings.meshtastic.pin);
    } else if (strcmp_P(what, PSTR("linux")) == 0) {
        gpio = system->getGpio(system->settings.linux.pin);
    } else if (strcmp_P(what, PSTR("wifi")) == 0) {
        gpio = system->getGpio(system->settings.linux.wifiPin);
    } else if (strcmp_P(what, PSTR("npr")) == 0) {
        gpio = system->getGpio(system->settings.linux.nprPin);
    } else {
        int pinNumber = atoi(what);
        if (pinNumber > 0) {
            Log.noticeln(F("[COMMAND_GPIO] Gpio tested as pin number for %d"), pinNumber);
            gpio = system->getGpio(pinNumber);
        }
    }

    if (gpio != nullptr) {
        gpio->setState(state);
        strcpy_P(response, PSTR("OK"));
        return;
    }

    Log.warningln(F("[COMMAND_GPIO] Gpio %s not found"), what);
    strcpy_P(response, PSTR("KO"));
}

void Command::doSetSetting(MyCommandParser::Argument *args, char *response) {
    char *key = args[0].asString;
    char *value = args[1].asString;

    if (strlen(value) == 0) {
        Log.warningln(F("[COMMAND] Set %s to nothing impossible"), key);
        strcpy_P(response, PSTR("KO"));
        return;
    }

    bool ok = true;
    bool shouldReboot = false;

    Log.infoln(F("[COMMAND] Set %s to %s"), key, value);

    if (strcmp_P(key, PSTR("lora.frequency")) == 0) {
        system->settings.lora.frequency = strtof(value, nullptr);
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.bandwidth")) == 0) {
        system->settings.lora.bandwidth = (uint16_t) strtoul(value, nullptr, 0);
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.spreadingFactor")) == 0) {
        system->settings.lora.spreadingFactor = (uint8_t) strtoul(value, nullptr, 0);
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.codingRate")) == 0) {
        system->settings.lora.codingRate = (uint8_t) strtoul(value, nullptr, 0);
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.outputPower")) == 0) {
        system->settings.lora.outputPower = (uint8_t) strtoul(value, nullptr, 0);
        ok = system->communication.begin();
    } else if (strcmp_P(key, PSTR("lora.txEnabled")) == 0) {
        system->settings.lora.txEnabled = value[0] == '1';
        system->watchdogSlaveLoraTxThread->feed();
    } else if (strcmp_P(key, PSTR("lora.watchdogTxEnabled")) == 0) {
        system->settings.lora.watchdogTxEnabled =
                system->watchdogSlaveLoraTxThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("lora.intervalTimeoutWatchdogTx")) == 0) {
        system->settings.lora.intervalTimeoutWatchdogTx = (uint64_t) strtoull(value, nullptr, 0);
        system->watchdogSlaveLoraTxThread->setInterval(system->settings.lora.intervalTimeoutWatchdogTx);
    } else if (strcmp_P(key, PSTR("aprs.call")) == 0) {
        strcpy(system->settings.aprs.call, value);
    } else if (strcmp_P(key, PSTR("aprs.destination")) == 0) {
        strcpy(system->settings.aprs.destination, value);
    } else if (strcmp_P(key, PSTR("aprs.path")) == 0) {
        strcpy(system->settings.aprs.path, value);
    } else if (strcmp_P(key, PSTR("aprs.comment")) == 0) {
        strcpy(system->settings.aprs.comment, value);
    } else if (strcmp_P(key, PSTR("aprs.status")) == 0) {
        strcpy(system->settings.aprs.status, value);
    } else if (strcmp_P(key, PSTR("aprs.symbol")) == 0) {
        system->settings.aprs.symbol = value[0];
    } else if (strcmp_P(key, PSTR("aprs.symbolTable")) == 0) {
        system->settings.aprs.symbolTable = value[0];
    } else if (strcmp_P(key, PSTR("aprs.latitude")) == 0) {
        system->settings.aprs.latitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("aprs.longitude")) == 0) {
        system->settings.aprs.longitude = strtod(value, nullptr);
    } else if (strcmp_P(key, PSTR("aprs.altitude")) == 0) {
        system->settings.aprs.altitude = (uint8_t) strtoul(value, nullptr, 0);
    } else if (strcmp_P(key, PSTR("aprs.digipeaterEnabled")) == 0) {
        system->settings.aprs.digipeaterEnabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.telemetryEnabled")) == 0) {
        system->settings.aprs.telemetryEnabled =
                system->sendTelemetriesThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.intervalTelemetry")) == 0) {
        system->settings.aprs.intervalTelemetry = (uint64_t) strtoull(value, nullptr, 0);
        system->sendTelemetriesThread->setInterval(system->settings.aprs.intervalTelemetry);
    } else if (strcmp_P(key, PSTR("aprs.statusEnabled")) == 0) {
        system->settings.aprs.statusEnabled =
                system->sendStatusThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.intervalStatus")) == 0) {
        system->settings.aprs.intervalStatus = (uint64_t) strtoull(value, nullptr, 0);
        system->sendStatusThread->setInterval(system->settings.aprs.intervalStatus);
    } else if (strcmp_P(key, PSTR("aprs.positionWeatherEnabled")) == 0) {
        system->settings.aprs.positionWeatherEnabled =
                system->sendPositionThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("aprs.intervalPositionWeather")) == 0) {
        system->settings.aprs.intervalPositionWeather = (uint64_t) strtoull(value, nullptr, 0);
        system->sendPositionThread->setInterval(system->settings.aprs.intervalPositionWeather);
    } else if (strcmp_P(key, PSTR("aprs.telemetryInPosition")) == 0) {
        system->settings.aprs.telemetryInPosition = value[0] == '1';
    } else if (strcmp_P(key, PSTR("meshtastic.watchdogEnabled")) == 0) {
        system->settings.meshtastic.watchdogEnabled =
                system->watchdogMeshtastic->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("meshtastic.intervalTimeoutWatchdog")) == 0) {
        system->settings.meshtastic.intervalTimeoutWatchdog = strtoull(value, nullptr, 0);
        system->watchdogMeshtastic->setInterval(system->settings.meshtastic.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("meshtastic.pin")) == 0) {
        system->settings.meshtastic.pin = (uint8_t) strtoul(value, nullptr, 0);
        if (system->watchdogMeshtastic->enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveEnabled")) == 0) {
        system->settings.meshtastic.i2cSlaveEnabled = value[0] == '1';
        if (system->settings.meshtastic.i2cSlaveEnabled) {
            I2CSlave::begin(system);
        } else {
            I2CSlave::end();
        }
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveAddress")) == 0) {
        system->settings.meshtastic.i2cSlaveAddress = (uint8_t) strtoul(value, nullptr, 0);
        if (system->settings.meshtastic.i2cSlaveEnabled) {
            I2CSlave::end();
            I2CSlave::begin(system);
        }
    } else if (strcmp_P(key, PSTR("meshtastic.aprsSendItemEnabled")) == 0) {
        system->settings.meshtastic.aprsSendItemEnabled =
        system->meshtasticSendAprsThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("meshtastic.intervalSendItem")) == 0) {
        system->settings.meshtastic.intervalSendItem = (uint64_t) strtoull(value, nullptr, 0);
        system->meshtasticSendAprsThread->setInterval(system->settings.meshtastic.intervalSendItem);
    } else if (strcmp_P(key, PSTR("meshtastic.itemName")) == 0) {
        strcpy(system->settings.meshtastic.itemName, value);
    } else if (strcmp_P(key, PSTR("meshtastic.itemComment")) == 0) {
        strcpy(system->settings.meshtastic.itemComment, value);
    } else if (strcmp_P(key, PSTR("meshtastic.symbol")) == 0) {
        system->settings.meshtastic.symbol = value[0];
    } else if (strcmp_P(key, PSTR("meshtastic.symbolTable")) == 0) {
        system->settings.meshtastic.symbolTable = value[0];
    } else if (strcmp_P(key, PSTR("mpptWatchdog.enabled")) == 0) {
        system->settings.mpptWatchdog.enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeout")) == 0) {
        system->settings.mpptWatchdog.timeout = (uint64_t) strtoull(value, nullptr, 0);
        if (system->settings.mpptWatchdog.enabled) {
            system->watchdogSlaveMpptChgThread->feed();
        }
    } else if (strcmp_P(key, PSTR("mpptWatchdog.intervalFeed")) == 0) {
        system->settings.mpptWatchdog.intervalFeed = (uint64_t) strtoull(value, nullptr, 0);
        system->watchdogSlaveMpptChgThread->setInterval(system->settings.mpptWatchdog.intervalFeed);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeOff")) == 0) {
        system->settings.mpptWatchdog.timeOff = (uint16_t) strtoul(value, nullptr, 0);
        if (system->settings.mpptWatchdog.enabled) {
            system->watchdogSlaveMpptChgThread->feed();
        }
    } else if (strcmp_P(key, PSTR("boxOpened.enabled")) == 0) {
        system->settings.boxOpened.enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("boxOpened.intervalCheck")) == 0) {
        system->settings.boxOpened.intervalCheck = (uint64_t) strtoull(value, nullptr, 0);
        system->ldrBoxOpenedThread->setInterval(system->settings.boxOpened.intervalCheck);
    } else if (strcmp_P(key, PSTR("boxOpened.pin")) == 0) {
        system->settings.boxOpened.pin = (uint8_t) strtoul(value, nullptr, 0);
        if (system->settings.boxOpened.enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("weather.enabled")) == 0) {
        system->settings.weather.enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("weather.intervalCheck")) == 0) {
        system->settings.weather.intervalCheck = (uint64_t) strtoull(value, nullptr, 0);
        system->weatherThread->setInterval(system->settings.weather.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.intervalCheck")) == 0) {
        system->settings.energy.intervalCheck = (uint64_t) strtoull(value, nullptr, 0);
        system->energyThread->setInterval(system->settings.energy.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.type")) == 0) {
        system->settings.energy.type = (TypeEnergySensor) strtoul(value, nullptr, 0);
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("energy.adcPin")) == 0) {
        system->settings.energy.adcPin = (uint8_t) strtoul(value, nullptr, 0);
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("energy.inaChannelBattery")) == 0) {
        system->settings.energy.inaChannelBattery =
                ((EnergyIna3221Thread*) system->energyThread)->channelBattery = (ina3221_ch_t) strtoul(value, nullptr, 0);
    } else if (strcmp_P(key, PSTR("energy.inaChannelSolar")) == 0) {
        system->settings.energy.inaChannelSolar =
                ((EnergyIna3221Thread*) system->energyThread)->channelSolar = (ina3221_ch_t) strtoul(value, nullptr, 0);
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOnVoltage")) == 0) {
        system->settings.energy.mpptPowerOnVoltage = (uint16_t) strtoul(value, nullptr, 0);
        ok = system->energyThread->begin();
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOffVoltage")) == 0) {
        system->settings.energy.mpptPowerOffVoltage = (uint16_t) strtoul(value, nullptr, 0);
        ok = system->energyThread->begin();
    } else if (strcmp_P(key, PSTR("linux.watchdogEnabled")) == 0) {
        system->settings.linux.watchdogEnabled =
                system->watchdogLinux->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("linux.intervalTimeoutWatchdog")) == 0) {
        system->settings.linux.intervalTimeoutWatchdog = (uint64_t) strtoull(value, nullptr, 0);
        system->watchdogLinux->setInterval(system->settings.linux.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("linux.pin")) == 0) {
        system->settings.linux.pin = (uint8_t) strtoul(value, nullptr, 0);
        if (system->watchdogLinux->enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("linux.nprPin")) == 0) {
        system->settings.linux.nprPin = (uint8_t) strtoul(value, nullptr, 0);
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("linux.wifiPin")) == 0) {
        system->settings.linux.wifiPin = (uint8_t) strtoul(value, nullptr, 0);
        system->planReboot();
        shouldReboot = true;
    } else if (strcmp_P(key, PSTR("linux.aprsSendItemEnabled")) == 0) {
        system->settings.linux.aprsSendItemEnabled =
        system->linuxSendAprsThread->enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("linux.intervalSendItem")) == 0) {
        system->settings.linux.intervalSendItem = (uint64_t) strtoull(value, nullptr, 0);
        system->linuxSendAprsThread->setInterval(system->settings.linux.intervalSendItem);
    } else if (strcmp_P(key, PSTR("linux.itemName")) == 0) {
        strcpy(system->settings.linux.itemName, value);
    } else if (strcmp_P(key, PSTR("linux.itemComment")) == 0) {
        strcpy(system->settings.linux.itemComment, value);
    } else if (strcmp_P(key, PSTR("linux.symbol")) == 0) {
        system->settings.linux.symbol = value[0];
    } else if (strcmp_P(key, PSTR("linux.symbolTable")) == 0) {
        system->settings.linux.symbolTable = value[0];
    } else if (strcmp_P(key, PSTR("rtc.enabled")) == 0) {
        system->settings.rtc.enabled = value[0] == '1';
    } else if (strcmp_P(key, PSTR("rtc.wakeUpPin")) == 0) {
        system->settings.rtc.wakeUpPin = (uint8_t) strtoul(value, nullptr, 0);
        if (system->settings.rtc.enabled) {
            system->planReboot();
            shouldReboot = true;
        }
    } else if (strcmp_P(key, PSTR("useInternalWatchdog")) == 0) {
        system->settings.useInternalWatchdog = value[0] == '1';
        system->planReboot();
        shouldReboot = true;
    } else {
        Log.warningln(F("[COMMAND] Config key not found"));
        ok = false;
    }

    if (ok) {
        ok = system->saveSettings();
        system->printSettings();
    }

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));

    if (shouldReboot) {
        strcat_P(response, PSTR(" Reboot"));
    }
}

void Command::doGetSetting(MyCommandParser::Argument *args, char *response) {
    char *key = args[0].asString;

    if (strcmp_P(key, PSTR("lora.frequency")) == 0) {
        sprintf_P(response, PSTR("%f"), system->settings.lora.frequency);
    } else if (strcmp_P(key, PSTR("lora.bandwidth")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.lora.bandwidth);
    } else if (strcmp_P(key, PSTR("lora.spreadingFactor")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.lora.spreadingFactor);
    } else if (strcmp_P(key, PSTR("lora.codingRate")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.lora.codingRate);
    } else if (strcmp_P(key, PSTR("lora.outputPower")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.lora.outputPower);
    } else if (strcmp_P(key, PSTR("lora.txEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.lora.txEnabled);
    } else if (strcmp_P(key, PSTR("lora.watchdogTxEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.lora.watchdogTxEnabled);
    } else if (strcmp_P(key, PSTR("lora.intervalTimeoutWatchdogTx")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.lora.intervalTimeoutWatchdogTx);
    } else if (strcmp_P(key, PSTR("aprs.call")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.aprs.call);
    } else if (strcmp_P(key, PSTR("aprs.destination")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.aprs.destination);
    } else if (strcmp_P(key, PSTR("aprs.path")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.aprs.path);
    } else if (strcmp_P(key, PSTR("aprs.comment")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.aprs.comment);
    } else if (strcmp_P(key, PSTR("aprs.status")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.aprs.status);
    } else if (strcmp_P(key, PSTR("aprs.symbol")) == 0) {
        sprintf_P(response, PSTR("%c"), system->settings.aprs.symbol);
    } else if (strcmp_P(key, PSTR("aprs.symbolTable")) == 0) {
        sprintf_P(response, PSTR("%c"), system->settings.aprs.symbolTable);
    } else if (strcmp_P(key, PSTR("aprs.latitude")) == 0) {
        sprintf_P(response, PSTR("%lf"), system->settings.aprs.latitude);
    } else if (strcmp_P(key, PSTR("aprs.longitude")) == 0) {
        sprintf_P(response, PSTR("%lf"), system->settings.aprs.longitude);
    } else if (strcmp_P(key, PSTR("aprs.altitude")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.aprs.altitude);
    } else if (strcmp_P(key, PSTR("aprs.digipeaterEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.aprs.digipeaterEnabled);
    } else if (strcmp_P(key, PSTR("aprs.telemetryEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.aprs.telemetryEnabled);
    } else if (strcmp_P(key, PSTR("aprs.intervalTelemetry")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.aprs.intervalTelemetry);
    } else if (strcmp_P(key, PSTR("aprs.statusEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.aprs.statusEnabled);
    } else if (strcmp_P(key, PSTR("aprs.intervalStatus")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.aprs.intervalStatus);
    } else if (strcmp_P(key, PSTR("aprs.positionWeatherEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.aprs.positionWeatherEnabled);
    } else if (strcmp_P(key, PSTR("aprs.intervalPositionWeather")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.aprs.intervalPositionWeather);
    } else if (strcmp_P(key, PSTR("aprs.telemetryInPosition")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.aprs.telemetryInPosition);
    } else if (strcmp_P(key, PSTR("meshtastic.watchdogEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.meshtastic.watchdogEnabled);
    } else if (strcmp_P(key, PSTR("meshtastic.intervalTimeoutWatchdog")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.meshtastic.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("meshtastic.pin")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.meshtastic.pin);
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.meshtastic.i2cSlaveEnabled);
    } else if (strcmp_P(key, PSTR("meshtastic.i2cSlaveAddress")) == 0) {
        sprintf_P(response, PSTR("0x%x"), system->settings.meshtastic.i2cSlaveAddress);
    } else if (strcmp_P(key, PSTR("meshtastic.aprsSendItemEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.meshtastic.aprsSendItemEnabled);
    } else if (strcmp_P(key, PSTR("meshtastic.intervalSendItem")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.meshtastic.intervalSendItem);
    } else if (strcmp_P(key, PSTR("meshtastic.itemName")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.meshtastic.itemName);
    } else if (strcmp_P(key, PSTR("meshtastic.itemComment")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.meshtastic.itemComment);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.enabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.mpptWatchdog.enabled);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeout")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.mpptWatchdog.timeout);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.intervalFeed")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.mpptWatchdog.intervalFeed);
    } else if (strcmp_P(key, PSTR("mpptWatchdog.timeOff")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.mpptWatchdog.timeOff);
    } else if (strcmp_P(key, PSTR("boxOpened.enabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.boxOpened.enabled);
    } else if (strcmp_P(key, PSTR("boxOpened.intervalCheck")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.boxOpened.intervalCheck);
    } else if (strcmp_P(key, PSTR("boxOpened.pin")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.boxOpened.pin);
    } else if (strcmp_P(key, PSTR("weather.enabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.weather.enabled);
    } else if (strcmp_P(key, PSTR("weather.intervalCheck")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.weather.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.intervalCheck")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.energy.intervalCheck);
    } else if (strcmp_P(key, PSTR("energy.type")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.energy.type);
    } else if (strcmp_P(key, PSTR("energy.adcPin")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.energy.adcPin);
    } else if (strcmp_P(key, PSTR("energy.inaChannelBattery")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.energy.inaChannelBattery);
    } else if (strcmp_P(key, PSTR("energy.inaChannelSolar")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.energy.inaChannelSolar);
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOnVoltage")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.energy.mpptPowerOnVoltage);
    } else if (strcmp_P(key, PSTR("energy.mpptPowerOffVoltage")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.energy.mpptPowerOffVoltage);
    } else if (strcmp_P(key, PSTR("linux.watchdogEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.linux.watchdogEnabled);
    } else if (strcmp_P(key, PSTR("linux.intervalTimeoutWatchdog")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.linux.intervalTimeoutWatchdog);
    } else if (strcmp_P(key, PSTR("linux.pin")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.linux.pin);
    } else if (strcmp_P(key, PSTR("linux.nprPin")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.linux.nprPin);
    } else if (strcmp_P(key, PSTR("linux.wifiPin")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.linux.wifiPin);
    } else if (strcmp_P(key, PSTR("linux.aprsSendItemEnabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.linux.aprsSendItemEnabled);
    } else if (strcmp_P(key, PSTR("linux.intervalSendItem")) == 0) {
        sprintf_P(response, PSTR("%llu"), system->settings.linux.intervalSendItem);
    } else if (strcmp_P(key, PSTR("linux.itemName")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.linux.itemName);
    } else if (strcmp_P(key, PSTR("linux.itemComment")) == 0) {
        sprintf_P(response, PSTR("%s"), system->settings.linux.itemComment);
    } else if (strcmp_P(key, PSTR("rtc.enabled")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.rtc.enabled);
    } else if (strcmp_P(key, PSTR("rtc.wakeUpPin")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.rtc.wakeUpPin);
    } else if (strcmp_P(key, PSTR("useInternalWatchdog")) == 0) {
        sprintf_P(response, PSTR("%d"), system->settings.useInternalWatchdog);
    } else if (strcmp_P(key, PSTR("all")) == 0) {
        system->printSettings();
        strcpy_P(response, PSTR("OK"));
    } else {
        Log.warningln(F("[COMMAND] Config key not found"));
        strcpy_P(response, PSTR("KO"));
    }
}

void Command::doPrintJson(MyCommandParser::Argument *args, char *response) {
    system->energyThread->run();

    if (system->weatherThread->enabled) {
        system->weatherThread->run();
    }

    if (system->ldrBoxOpenedThread->enabled) {
        system->ldrBoxOpenedThread->run();
    }

    delayWdt(1000);

    if (system->watchdogLinux->enabled) {
        system->watchdogLinux->feed();
    }

    system->printJson(false);
    system->printJson(true);

    strcpy_P(response, PSTR(""));
}

void Command::doPing(MyCommandParser::Argument *args, char *response) {
    Log.infoln(F("Pong !"));
    strcpy_P(response, PSTR("Pong!"));
}

void Command::doMpptWatchdog(MyCommandParser::Argument *args, char *response) {
    uint64_t timeOff = args[0].asUInt64;

    bool ok = system->watchdogSlaveMpptChgThread->enabled && system->watchdogSlaveMpptChgThread->setManagedByUser(timeOff);

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));
}

void Command::doLinuxWatchdog(MyCommandParser::Argument *args, char *response) {
    bool ok = system->settings.linux.watchdogEnabled && system->watchdogLinux->feed();

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));
}

void Command::doSetTime(MyCommandParser::Argument *args, char *response) {
    uint64_t epoch = args[0].asUInt64 + 15;

    if (system->settings.rtc.enabled) {
        system->rtc.setEpoch((long long) epoch, true);
    }

    system->setTimeToInternalRtc(epoch);

    strcpy_P(response, PSTR("OK"));
}

void Command::doMeshtasticAprs(MyCommandParser::Argument *args, char *response) {
    system->meshtasticSendAprsThread->forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doLinuxAprs(MyCommandParser::Argument *args, char *response) {
    system->linuxSendAprsThread->forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doGetLdrAdc(MyCommandParser::Argument *args, char *response) {
    sprintf_P(response, PSTR("%d OK"), system->ldrBoxOpenedThread->getRawValue());
}
