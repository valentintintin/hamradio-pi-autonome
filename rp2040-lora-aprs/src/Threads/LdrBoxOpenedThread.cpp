#include "Threads/LdrBoxOpenedThread.h"
#include "variant.h"
#include "System.h"
#include "ArduinoLog.h"

LdrBoxOpenedThread::LdrBoxOpenedThread(System *system, GpioPin *ldr) : MyThread(system, INTERVAL_BOX_OPENED, PSTR("LDR_BOX_OPENED")), ldr(ldr) {}

bool LdrBoxOpenedThread::runOnce() {
    if (millis() < INTERVAL_BOX_OPENED) {
        return true;
    }

    if (ldr->getState()) {
        Log.warningln(F("[LDR_BOX_OPENED] LDR box opened !"));
        return system->communication.sendMessage(PSTR("F4HVV-7"), PSTR("Box opened !"));
    }

    Log.traceln(F("[LDR_BOX_OPENED] Box closed"));
    return true;
}