#ifndef RP2040_LORA_APRS_ENERGYDUMMY_H
#define RP2040_LORA_APRS_ENERGYDUMMY_H

#include "Threads/EnergyThread.h"

class EnergyDummy : public EnergyThread {
public:
    explicit EnergyDummy(System* system);
protected:
    bool init() override;
    bool fetchVoltageBattery() override;
    bool fetchCurrentBattery() override;
    bool fetchVoltageSolar() override;
    bool fetchCurrentSolar() override;
};


#endif //RP2040_LORA_APRS_ENERGYDUMMY_H
