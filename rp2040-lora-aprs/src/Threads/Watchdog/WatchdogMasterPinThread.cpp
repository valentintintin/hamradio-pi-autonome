#include "Threads/Watchdog/WatchdogMasterPinThread.h"

#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"

WatchdogMasterPinThread::WatchdogMasterPinThread(System *system, GpioPin *gpio, const unsigned long intervalCheck, const bool enabled):
WatchdogThread(system, intervalCheck, PSTR("WATCHDOG_PIN"), enabled), gpio(gpio) {
}

bool WatchdogMasterPinThread::runOnce() {
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

    if (!gpio->getState()) {
        Log.warningln(F("[WATCHDOG_PIN_%d] Dog not fed but pin low"), gpio->getPin());
        return true;
    }

    Log.warningln(F("[WATCHDOG_PIN_%d] Dog not fed so toggle pin"), gpio->getPin());

    gpio->setState(LOW);
    delayWdt(TIME_WAIT_TOGGLE_WATCHDOG_MASTER);
    gpio->setState(HIGH);

    return true;
}

bool WatchdogMasterPinThread::feed() {
    Log.infoln(F("[WATCHDOG_PIN_%d] Feed dog"), gpio->getPin());
    hasFed = true;
    lastFed = millis();
    return true;
}

void WatchdogMasterPinThread::sleep(const uint64_t millis) {
    Log.infoln(F("[WATCHDOG_PIN_%d] Sleep for %ums at next internal (%u)"), gpio->getPin(), millis, interval);
    timerSleep.setInterval(millis);
    wantToSleep = true;
}