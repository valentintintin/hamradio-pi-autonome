#ifndef RP2040_LORA_APRS_WATCHDOGTHREAD_H
#define RP2040_LORA_APRS_WATCHDOGTHREAD_H

#include "MyThread.h"

#define WATCHDOG_TIME_AFTER_BOOT 60000 // 1 minute

class System;

class WatchdogThread : public MyThread {
public:
    explicit WatchdogThread(System *system, unsigned long interval, const char *name);
    virtual bool feed() = 0;
};


#endif //RP2040_LORA_APRS_WATCHDOGTHREAD_H
