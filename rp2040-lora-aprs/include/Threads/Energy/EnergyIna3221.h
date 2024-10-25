#ifndef RP2040_LORA_APRS_ENERGYINA3221_H
#define RP2040_LORA_APRS_ENERGYINA3221_H

#include "Threads/EnergyThread.h"
#include "INA3221.h"

class EnergyIna3221 : public EnergyThread {
public:
    explicit EnergyIna3221(System *system, ina3221_ch_t channelBattery, ina3221_ch_t channelSolar);
protected:
    bool init() override;
    bool fetchVoltageBattery() override;
    bool fetchCurrentBattery() override;
    bool fetchVoltageSolar() override;
    bool fetchCurrentSolar() override;
private:
    INA3221 ina3221 = INA3221(INA3221_ADDR42_SDA);
    ina3221_ch_t channelBattery;
    ina3221_ch_t channelSolar;
};


#endif //RP2040_LORA_APRS_ENERGYINA3221_H
