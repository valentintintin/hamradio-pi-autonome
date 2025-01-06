#include "Threads/WatchdogThread.h"

WatchdogThread::WatchdogThread(System *system, unsigned long interval, const char *name, bool enabled) : MyThread(system, interval, name, false, enabled) {
    force = true;
}

bool WatchdogThread::shouldRun(unsigned long time) {
    return MyThread::shouldRun(time) && millis() > TIME_AFTER_BOOT;
}