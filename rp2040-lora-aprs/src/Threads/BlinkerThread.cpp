#include "Threads/BlinkerThread.h"
#include "ArduinoLog.h"
#include "utils.h"
#include "variant.h"

BlinkerThread::BlinkerThread(System *system, GpioPin *gpio) : MyThread(system, INTERVAL_BLINKER, PSTR("BLINKER")), gpio(gpio) {
}

bool BlinkerThread::runOnce() {
    gpio->setState(HIGH);
    delayWdt(250);
    gpio->setState(LOW);
    delayWdt(250);

    return true;
}