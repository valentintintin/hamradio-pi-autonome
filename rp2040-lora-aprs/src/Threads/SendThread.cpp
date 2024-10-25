#include "Threads/SendThread.h"
#include "System.h"
#include "ArduinoLog.h"

SendThread::SendThread(System *system, unsigned long interval, const char *name) : MyThread(system, interval, name) {
}

bool SendThread::runOnce() {
    if (send()) {
        force = false;
        return true;
    }

    return false;
}

bool SendThread::shouldRun(unsigned long time) {
    return Thread::shouldRun(time) || force;
}

void SendThread::forceRun() {
    Log.infoln(F("[SEND_%s] Force send"), ThreadName.c_str());
    force = true;
}
