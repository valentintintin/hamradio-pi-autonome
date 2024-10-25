#ifndef RP2040_LORA_APRS_WATCHDOGSLAVELORATX_H
#define RP2040_LORA_APRS_WATCHDOGSLAVELORATX_H

#include "Threads/WatchdogThread.h"

class WatchdogSlaveLoraTx : public WatchdogThread {
public:
    explicit WatchdogSlaveLoraTx(System *system);
    bool feed() override;
protected:
    bool runOnce() override;
private:
    bool hasTx = false;
};


#endif //RP2040_LORA_APRS_WATCHDOGSLAVELORATX_H
