#ifndef RP2040_LORA_APRS_LDRBOXOPENEDTHREAD_H
#define RP2040_LORA_APRS_LDRBOXOPENEDTHREAD_H

#include "MyThread.h"
#include "GpioPin.h"

class System;

class LdrBoxOpenedThread : public MyThread {
public:
    explicit LdrBoxOpenedThread(System *system, GpioPin *ldr);
protected:
    bool runOnce() override;
private:
    GpioPin *ldr;
};


#endif //RP2040_LORA_APRS_LDRBOXOPENEDTHREAD_H