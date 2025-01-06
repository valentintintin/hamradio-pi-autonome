#include "Threads/SendThread.h"
#include "System.h"
#include "ArduinoLog.h"

SendThread::SendThread(System *system, unsigned long interval, const char *name, bool enabled) : MyThread(system, interval, name, false, enabled) {
}

bool SendThread::shouldRun(unsigned long time) {
    return MyThread::shouldRun(time) && isAfterBoot();
}