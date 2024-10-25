#include "ArduinoLog.h"

#include "Command.h"
#include "System.h"

#ifdef USE_MPPTCHG
    #include "Threads/Energy/EnergyMpptChg.h"
#endif

System* Command::system;

Command::Command(System *system) {
    Command::system = system;

    parser.registerCommand(PSTR("position"), PSTR(""), doPosition);
    parser.registerCommand(PSTR("telem"), PSTR(""), doTelemetry);
    parser.registerCommand(PSTR("telemParams"), PSTR(""), doTelemetryParams);
    parser.registerCommand(PSTR("lora"), PSTR("s"), doLora);
    parser.registerCommand(PSTR("reset"), PSTR(""), doReset);
    parser.registerCommand(PSTR("json"), PSTR(""), doGetJson);
    parser.registerCommand(PSTR("ping"), PSTR(""), doPing);

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
}

bool Command::processCommand(const char *command) {
    if (strlen(command) < 3) {
        Log.traceln(F("[COMMAND] Command received length %d : %s"), strlen(command), command);
        return false;
    }

    Log.traceln(F("[COMMAND] Process : %s"), command);

    if (!parser.processCommand(command, response)) {
        return false;
    }

    Log.infoln(F("[COMMAND] %s = %s"), command, response);

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

    // todo remake
    system->communication.sendMessage(PSTR(APRS_DESTINATION), message);

    strcpy_P(response, PSTR("OK"));
}

void Command::doReset(MyCommandParser::Argument *args, char *response) {
    rp2040.reboot();

    strcpy_P(response, PSTR("OK"));
}

void Command::doGetJson(MyCommandParser::Argument *args, char *response) {
    strcpy_P(response, PSTR("OK"));
}

void Command::doPing(MyCommandParser::Argument *args, char *response) {
    Log.infoln(F("Pong !"));
    strcpy_P(response, PSTR("Pong!"));
}

#ifdef USE_MESHTASTIC
void Command::doMeshtastic(MyCommandParser::Argument *args, char *response) {
    system->gpioMeshtastic.setState(args[0].asUInt64 == 0);
    strcpy_P(response, PSTR("Pong!"));
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