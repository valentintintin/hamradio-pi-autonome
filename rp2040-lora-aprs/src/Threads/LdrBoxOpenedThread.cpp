#include "Threads/LdrBoxOpenedThread.h"
#include "System.h"
#include "ArduinoLog.h"

LdrBoxOpenedThread::LdrBoxOpenedThread(System *system) : MyThread(system, system->settings.boxOpened.intervalCheck, PSTR("LDR_BOX_OPENED")), ldr(new GpioPin(system->settings.boxOpened.pin, INPUT)) {
    enabled = system->settings.boxOpened.enabled;
}

bool LdrBoxOpenedThread::runOnce() {
    if (millis() < system->settings.boxOpened.intervalCheck) {
        return true;
    }

    _isBoxOpened = ldr->getState();

    if (_isBoxOpened) {
        Log.warningln(F("[LDR_BOX_OPENED] LDR box opened !"));
        return system->communication.sendMessage(PSTR("F4HVV-7"), PSTR("Box opened !"));
    }

    Log.traceln(F("[LDR_BOX_OPENED] Box closed"));
    return true;
}