#ifndef RP2040_LORA_APRS_SYSTEM_H
#define RP2040_LORA_APRS_SYSTEM_H

#include <cstdint>
#include <ThreadController.h>

#include "Communication.h"
#include "Timer.h"
#include "config.h"
#include "Command.h"
#include "GpioPin.h"
#include "Threads/EnergyThread.h"
#include "Threads/WeatherThread.h"
#include "Threads/Watchdog/WatchdogSlaveMpptChg.h"
#include "Threads/Watchdog/WatchdogSlaveLoraTx.h"
#include "Threads/Watchdog/WatchdogMasterPin.h"
#include "Threads/Send/SendPositionThread.h"
#include "Threads/Send/SendStatusThread.h"
#include "Threads/Send/SendTelemetriesThread.h"

#include "variant.h"

class System {
public:
    explicit System();

    bool begin();
    void loop();

    inline bool hasError() {
        return communication.hasError() || weatherThread.hasError() || energyThread->hasError();
    }

    EnergyThread *energyThread;
    WeatherThread weatherThread;
    WatchdogSlaveLoraTx watchdogSlaveLoraTx;
    SendPositionThread sendPositionThread;
    SendStatusThread sendStatusThread;
    SendTelemetriesThread sendTelemetriesThread;

    Communication communication;
    Command command;
    GpioPin gpioLed;

#if defined(USE_MPPTCHG) || defined(USE_MPPTCHG_WATCHDOG)
    mpptChg mpptChgCharger;
#endif
#ifdef USE_MPPTCHG_WATCHDOG
    WatchdogSlaveMpptChg *watchdogSlaveMpptChg;
#endif

#ifdef USE_MESHTASTIC
    WatchdogMasterPin *watchdogMeshtastic;
    GpioPin gpioMeshtastic = GpioPin(MESHTASTIC_PIN);
#endif

#ifdef USE_WATCHDOG_LINUX_BOARD
    WatchdogMasterPin *watchdogLinuxBoard;
    GpioPin gpioLinuxBoard = GpioPin(LINUX_BOARD_PIN);
#endif
private:
    ThreadController threadController;
};

#endif //RP2040_LORA_APRS_SYSTEM_H