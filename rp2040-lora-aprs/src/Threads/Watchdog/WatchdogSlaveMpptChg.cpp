#include "Threads/Watchdog/WatchdogSlaveMpptChg.h"

#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"
#include "variant.h"

WatchdogSlaveMpptChg::WatchdogSlaveMpptChg(System *system) :
WatchdogThread(system, WATCHDOG_MPPTCHG_INTERVAL_FEED_DOG, PSTR("WATCHDOG_MPPTCHG")) {
#ifdef USE_MPPTCHG_WATCHDOG
    charger = &system->mpptChgCharger;
#else
    assert(charger != nullptr);
#endif
}

bool WatchdogSlaveMpptChg::init() {
    if (charger->begin()
    && charger->setWatchdogPoweroff(WATCHDOG_MPPTCHG_TIME_POWER_OFF)
    && charger->setWatchdogTimeout(WATCHDOG_MPPTCHG_TIMEOUT)
    && charger->setWatchdogEnable(true)) {
        Log.noticeln(F("[WATCHDOG_MPPTCHG] Initiated with power off %d and timeout %d"), WATCHDOG_MPPTCHG_TIME_POWER_OFF, WATCHDOG_MPPTCHG_TIMEOUT);
        return true;
    }

    return false;
}

bool WatchdogSlaveMpptChg::runOnce() {
    return managedByUser || feed();
}

bool WatchdogSlaveMpptChg::feed() {
    if (!charger->setWatchdogTimeout(WATCHDOG_MPPTCHG_TIMEOUT)) {
        Log.errorln(F("[WATCHDOG_MPPTCHG] Fail to feed dog"));
        return false;
    }

    Log.infoln(F("[WATCHDOG_MPPTCHG] Dog fed"));

    return true;
}

bool WatchdogSlaveMpptChg::setManagedByUser(unsigned long millis) {
    if (millis == 0) {
        managedByUser = true;
        return charger->setWatchdogEnable(false);
    }

    if (millis == 1) {
        managedByUser = false;
        return feed();
    }

    managedByUser = true;
    return charger->setWatchdogPoweroff(millis)
           && charger->setWatchdogTimeout(10)
           && charger->setWatchdogEnable(true);
}