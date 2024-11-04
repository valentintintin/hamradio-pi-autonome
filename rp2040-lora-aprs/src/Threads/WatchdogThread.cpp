#include "Threads/WatchdogThread.h"

WatchdogThread::WatchdogThread(System *system, unsigned long interval, const char *name) : MyThread(system, interval, name) {}