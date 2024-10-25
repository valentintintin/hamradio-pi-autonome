#ifndef RP2040_LORA_APRS_WATCHDOGSLAVEMPPTCHG_H
#define RP2040_LORA_APRS_WATCHDOGSLAVEMPPTCHG_H

#include "Threads/WatchdogThread.h"
#include "mpptChg.h"

class WatchdogSlaveMpptChg : public WatchdogThread {
public:
    explicit WatchdogSlaveMpptChg(System *system);
    bool feed() override;
    bool setManagedByUser(unsigned long millis);
protected:
    bool init() override;
    bool runOnce() override;
private:
    mpptChg *charger;
    bool managedByUser = false;
};


#endif //RP2040_LORA_APRS_WATCHDOGSLAVEMPPTCHG_H
