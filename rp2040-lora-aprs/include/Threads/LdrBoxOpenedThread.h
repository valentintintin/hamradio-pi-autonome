#ifndef RP2040_LORA_APRS_LDRBOXOPENEDTHREAD_H
#define RP2040_LORA_APRS_LDRBOXOPENEDTHREAD_H

#include "MyThread.h"
#include "GpioPin.h"

class System;

class LdrBoxOpenedThread : public MyThread {
public:
    explicit LdrBoxOpenedThread(System *system);

    inline bool isBoxOpened() const {
        return _isBoxOpened;
    }

    inline uint16_t getRawValue() const {
        return ldr->getValue();
    }
protected:
    bool runOnce() override;
private:
    GpioPin *ldr;
    bool _isBoxOpened;
};


#endif //RP2040_LORA_APRS_LDRBOXOPENEDTHREAD_H
