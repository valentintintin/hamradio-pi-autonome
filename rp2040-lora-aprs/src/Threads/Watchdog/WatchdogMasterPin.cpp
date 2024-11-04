#include "Threads/Watchdog/WatchdogMasterPin.h"

#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"

WatchdogMasterPin::WatchdogMasterPin(System *system, GpioPin *gpio, unsigned long intervalCheck) : WatchdogThread(system, intervalCheck, PSTR("WATCHDOG_PIN")), gpio(gpio) {
    gpio->setState(HIGH);
}

bool WatchdogMasterPin::runOnce() {
    if (millis() < WATCHDOG_TIME_AFTER_BOOT) {
        return true;
    }

    if (wantToSleep) { // User ask to sleep
        if (gpio->getState()) { // Currently running
            gpio->setState(LOW); // Shutdown
            timerSleep.restart(); // Start timer
        } else if (timerSleep.hasExpired()) { // Not running and timer expired
            gpio->setState(HIGH); // Turn on
            wantToSleep  = false; // Reset state
            hasFed = false;
        }

        return true;
    }

    if (hasFed) {
        hasFed = false;
        Log.traceln(F("[WATCHDOG_PIN_%d] Dog fed"), gpio->getPin());
        return true;
    }

    Log.warningln(F("[WATCHDOG_PIN_%d] Dog not fed so toggle pin"), gpio->getPin());

    gpio->setState(LOW);
    delayWdt(TIME_WAIT_TOGGLE);
    gpio->setState(HIGH);

    return true;
}

bool WatchdogMasterPin::feed() {
    Log.infoln(F("[WATCHDOG_PIN_%d] Feed dog"), gpio->getPin());
    hasFed = true;
    return true;
}

void WatchdogMasterPin::sleep(unsigned long millis) {
    Log.infoln(F("[WATCHDOG_PIN_%d] Sleep for %ums at next internal (%u)"), gpio->getPin(), millis, interval);
    timerSleep.setInterval(millis);
    wantToSleep = true;
}