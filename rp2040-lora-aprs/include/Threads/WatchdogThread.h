#ifndef RP2040_LORA_APRS_WATCHDOGTHREAD_H
#define RP2040_LORA_APRS_WATCHDOGTHREAD_H

#include "MyThread.h"

class System;

class WatchdogThread : public MyThread {
public:
    explicit WatchdogThread(System *system, unsigned long interval, const char *name);
    virtual bool feed() = 0;
};


#endif //RP2040_LORA_APRS_WATCHDOGTHREAD_H
