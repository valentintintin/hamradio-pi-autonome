#include "Threads/WatchdogThread.h"
#include "ArduinoLog.h"

WatchdogThread::WatchdogThread(System *system, unsigned long interval, const char *name) : MyThread(system, interval, name) {}