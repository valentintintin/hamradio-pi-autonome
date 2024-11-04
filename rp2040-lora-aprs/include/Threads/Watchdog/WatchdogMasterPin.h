#ifndef RP2040_LORA_APRS_WATCHDOGMASTERPIN_H
#define RP2040_LORA_APRS_WATCHDOGMASTERPIN_H

#include "Threads/WatchdogThread.h"
#include "GpioPin.h"
#include "Timer.h"

#define TIME_WAIT_TOGGLE 10000 // 10 seconds

class WatchdogMasterPin : public WatchdogThread {
public:
    explicit WatchdogMasterPin(System *system, GpioPin *gpio, unsigned long intervalCheck);
    void sleep(unsigned long millis);
    bool feed() override;
protected:
    bool runOnce() override;
private:
    GpioPin *gpio;
    Timer timerSleep;
    bool hasFed = false;
    bool wantToSleep = false;
};


#endif //RP2040_LORA_APRS_WATCHDOGMASTERPIN_H
