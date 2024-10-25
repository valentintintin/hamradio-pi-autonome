#ifndef RP2040_LORA_APRS_SENDTHREAD_H
#define RP2040_LORA_APRS_SENDTHREAD_H

#include "MyThread.h"

class System;

class SendThread : public MyThread {
public:
    explicit SendThread(System *system, unsigned long interval, const char *name);
    bool shouldRun(unsigned long time) override;
    void forceRun();
protected:
    bool runOnce() override;
    virtual bool send() = 0;
private:
    bool force = false;
};

#endif //RP2040_LORA_APRS_SENDTHREAD_H
