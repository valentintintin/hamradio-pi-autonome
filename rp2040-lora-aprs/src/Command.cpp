#include "ArduinoLog.h"

#include "Command.h"
#include "System.h"
#include "utils.h"

#ifdef USE_MPPTCHG
    #include "Threads/Energy/EnergyMpptChg.h"
#endif

System* Command::system;
JsonWriter Command::serialJsonWriter(&Serial1);

Command::Command(System *system) {
    Command::system = system;

    parser.registerCommand(PSTR("position"), PSTR(""), doPosition);
    parser.registerCommand(PSTR("telem"), PSTR(""), doTelemetry);
    parser.registerCommand(PSTR("telemParams"), PSTR(""), doTelemetryParams);
    parser.registerCommand(PSTR("lora"), PSTR("s"), doLora);
    parser.registerCommand(PSTR("reboot"), PSTR(""), doReboot);
    parser.registerCommand(PSTR("dfu"), PSTR(""), doDfu);
    parser.registerCommand(PSTR("json"), PSTR(""), doGetJson);
    parser.registerCommand(PSTR("ping"), PSTR(""), doPing);
    parser.registerCommand(PSTR("gpio"), PSTR("uu"), doGpioOutput);

#ifdef USE_MESHTASTIC
    parser.registerCommand(PSTR("msh"), PSTR("u"), doMeshtastic);
#endif

#ifdef USE_MPPTCHG_WATCHDOG
    parser.registerCommand(PSTR("dog"), PSTR("u"), doMpptWatchdog);
#endif
#ifdef USE_MPPTCHG
    parser.registerCommand(PSTR("pow"), PSTR("uu"), doMpptPower);
#endif

#ifdef USE_WATCHDOG_LINUX_BOARD
    parser.registerCommand(PSTR("linuxDog"), PSTR(""), doLinuxWatchdog);
#endif

#ifdef USE_RTC
    parser.registerCommand(PSTR("time"), PSTR("u"), doSetTime);
#endif
}

bool Command::processCommand(Stream* stream, const char *command) {
    if (strlen(command) < 3) {
        Log.traceln(F("[COMMAND] Command received length %d : %s"), strlen(command), command);
        return false;
    }

    Log.traceln(F("[COMMAND] Process : %s"), command);

    if (!parser.processCommand(command, response)) {
        if (stream != nullptr) {
            stream->print(F("KO "));
            stream->println(response);
        }

        Log.warningln(F("[COMMAND] %s KO (%s)"), command, response);
        return false;
    }

    if (stream != nullptr && strlen(response)) {
        stream->println(response);
    }

    Log.infoln(F("[COMMAND] %s OK (%s)"), command, response);
    return true;
}

void Command::doTelemetry(MyCommandParser::Argument *args, char *response) {
    system->sendTelemetriesThread.forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doPosition(MyCommandParser::Argument *args, char *response) {
    system->sendPositionThread.forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doTelemetryParams(MyCommandParser::Argument *args, char *response) {
    system->communication.shouldSendTelemetryParams = true;
    system->sendTelemetriesThread.forceRun();

    strcpy_P(response, PSTR("OK"));
}

void Command::doLora(MyCommandParser::Argument *args, char *response) {
    char *message = args[0].asString;

    // todo remake ?
    system->communication.sendMessage(PSTR(APRS_DESTINATION), message);

    strcpy_P(response, PSTR("OK"));
}

void Command::doReboot(MyCommandParser::Argument *args, char *response) {
    rp2040.reboot();

    strcpy_P(response, PSTR("OK"));
}

void Command::doDfu(MyCommandParser::Argument *args, char *response) {
#ifdef USE_MPPTCHG_WATCHDOG
    system->watchdogSlaveMpptChg->setManagedByUser(TIME_WATCHDOG_DFU);
#endif

    rp2040.rebootToBootloader();

    strcpy_P(response, PSTR("OK"));
}

void Command::doGpioOutput(MyCommandParser::Argument *args, char *response) {
    auto pin = args[0].asUInt64;
    auto state = args[1].asUInt64 == 1;
    for (auto gpio : system->gpiosPin) {
        if (gpio == nullptr) {
            continue;
        }

        if (gpio->getPin() == pin) {
            gpio->setState(state);
            strcpy_P(response, PSTR("OK"));
            return;
        }
    }

    Log.warningln(F("[COMMAND_GPIO] Gpio %d not found"), pin);
    strcpy_P(response, PSTR("KO"));
}

void Command::doGetJson(MyCommandParser::Argument *args, char *response) {
    delayWdt(1000);

#ifdef USE_MPPTCHG
    uint16_t* temperatureBattery;
    if (!system->mpptChgCharger.getConfigurationValue(VAL_INT_TEMP, &temperatureBattery)) {
        Log.warningln(F("[COMMAND] Impossible to get MPPT Temperature"));
    }
#endif

    serialJsonWriter.beginObject()
            .property(F("uptime"), millis() / 1000)
            .property(F("time"), RTClib::now().unixtime())
            .property(F("voltageBattery"), system->energyThread->getVoltageBattery())
            .property(F("currentBattery"), system->energyThread->getCurrentBattery())
            .property(F("voltageSolar"), system->energyThread->getVoltageSolar())
            .property(F("currentSolar"), system->energyThread->getCurrentBattery())
#ifdef USE_RTC
            .property(F("temperatureRtc"), system->rtc.getTemperature())
#endif
#ifdef USE_MPPTCHG
            .property(F("temperatureBattery"), temperatureBattery / 10.0f)
#endif
#ifdef USE_WEATHER
            .property(F("temperature"), system->weatherThread.getTemperature())
            .property(F("humidity"), system->weatherThread.getHumidity())
            .property(F("pressure"), system->weatherThread.getPressure())
#endif
            .endObject();

#ifdef USE_WATCHDOG_LINUX_BOARD
    system->watchdogLinuxBoard->feed();
#endif

    strcpy_P(response, PSTR(""));
}

void Command::doPing(MyCommandParser::Argument *args, char *response) {
    Log.infoln(F("Pong !"));
    strcpy_P(response, PSTR("Pong!"));
}

#ifdef USE_MESHTASTIC
void Command::doMeshtastic(MyCommandParser::Argument *args, char *response) {
    system->gpioMeshtastic.setState(args[0].asUInt64 == 1);
    strcpy_P(response, PSTR("OK"));
}
#endif

#ifdef USE_MPPTCHG_WATCHDOG
void Command::doMpptWatchdog(MyCommandParser::Argument *args, char *response) {
    uint64_t timeOff = args[0].asUInt64;

    bool ok = system->watchdogSlaveMpptChg->setManagedByUser(timeOff);

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));
}
#endif

#ifdef USE_MPPTCHG
void Command::doMpptPower(MyCommandParser::Argument *args, char *response) {
    uint64_t powerOnVoltage = args[0].asUInt64;
    uint64_t powerOffVoltage = args[1].asUInt64;

    bool ok = ((EnergyMpptChg*) system->energyThread)->setPowerOnOff(powerOnVoltage, powerOffVoltage);

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));
}
#endif

#ifdef USE_WATCHDOG_LINUX_BOARD
void Command::doLinuxWatchdog(MyCommandParser::Argument *args, char *response) {
    bool ok = system->watchdogLinuxBoard->feed();

    strcpy_P(response, ok ? PSTR("OK") : PSTR("KO"));
}
#endif

#ifdef USE_RTC
void Command::doSetTime(MyCommandParser::Argument *args, char *response) {
    uint64_t epoch = args[0].asUInt64;

    system->rtc.setEpoch((long long) epoch, true);
    system->setTimeToRtc();

    strcpy_P(response, PSTR("OK"));
}

#endif