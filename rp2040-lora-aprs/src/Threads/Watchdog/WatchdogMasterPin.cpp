#include "Threads/Watchdog/WatchdogMasterPin.h"

#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"

WatchdogMasterPin::WatchdogMasterPin(System *system, GpioPin *gpio, unsigned long intervalCheck) : WatchdogThread(system, intervalCheck, PSTR("WATCHDOG_PIN")), gpio(gpio) {
    gpio->setState(HIGH);
    lastState = true;
}

bool WatchdogMasterPin::runOnce() {
    if (wantToSleep) {
        if (lastState) {
            gpio->setState(LOW);
            lastState = false;
            timerSleep.restart();
        } else if (timerSleep.hasExpired()) {
            gpio->setState(HIGH);
            lastState = true;
            wantToSleep  = false;
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
    lastState = false;
    delayWdt(TIME_WAIT_TOGGLE);
    gpio->setState(HIGH);
    lastState = true;

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
