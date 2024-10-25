#ifndef RP2040_LORA_APRS_MYTHREAD_H
#define RP2040_LORA_APRS_MYTHREAD_H

#include "Thread.h"

class System;

class MyThread : public Thread {
public:
    explicit MyThread(System *system, unsigned long interval, const char *name);
    bool begin();
    void run() override;

    inline bool hasError() const {
        return lastUpdateHasError;
    }
protected:
    virtual bool init() {
        return true;
    }

    virtual bool runOnce() = 0;

    System *system;
private:
    bool initiated = false;
    bool lastUpdateHasError = false;
};


#endif //RP2040_LORA_APRS_MYTHREAD_H
