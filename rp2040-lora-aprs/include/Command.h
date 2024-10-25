#ifndef CUBECELL_MONITORING_COMMAND_H
#define CUBECELL_MONITORING_COMMAND_H

#include <CommandParser.h>
#include "variant.h"

class System;

typedef CommandParser<16, 2, 16, 250, 0, 32> MyCommandParser;

class Command {
public:
    explicit Command(System *system);

    bool processCommand(const char *command);
private:
    static System *system;

    MyCommandParser parser;
    char response[MyCommandParser::MAX_RESPONSE_SIZE]{};

    static void doPosition(MyCommandParser::Argument *args, char *response);
    static void doTelemetry(MyCommandParser::Argument *args, char *response);
    static void doTelemetryParams(MyCommandParser::Argument *args, char *response);
    static void doLora(MyCommandParser::Argument *args, char *response);
    static void doReset(MyCommandParser::Argument *args, char *response);
    static void doGetJson(MyCommandParser::Argument *args, char *response);
    static void doPing(MyCommandParser::Argument *args, char *response);

#ifdef USE_MESHTASTIC
    static void doMeshtastic(MyCommandParser::Argument *args, char *response);
#endif

#ifdef USE_MPPTCHG_WATCHDOG
    static void doMpptWatchdog(MyCommandParser::Argument *args, char *response);
#endif
#ifdef USE_MPPTCHG
    static void doMpptPower(MyCommandParser::Argument *args, char *response);
#endif

#ifdef USE_WATCHDOG_LINUX_BOARD
    static void doMpptPower(MyCommandParser::Argument *args, char *response);
#endif

#ifdef USE_WATCHDOG_LINUX_BOARD
    static void doLinuxWatchdog(MyCommandParser::Argument *args, char *response);
#endif
};

#endif //CUBECELL_MONITORING_COMMAND_H
