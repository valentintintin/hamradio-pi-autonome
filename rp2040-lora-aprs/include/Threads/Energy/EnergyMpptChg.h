#ifndef RP2040_LORA_APRS_ENERGYMPPTCHG_H
#define RP2040_LORA_APRS_ENERGYMPPTCHG_H

#include "Threads/EnergyThread.h"

class EnergyMpptChg : public EnergyThread {
public:
    explicit EnergyMpptChg(System *system);
    void run() override;

    bool setPowerOnOff(uint16_t powerOnVoltage, uint16_t powerOffVoltage);

    inline bool isNight() const override {
        return _isNight;
    }

    inline uint16_t getStatus() const {
        return status;
    }

    inline uint16_t getPowerOffVoltage() const {
        return powerOffVoltage;
    }

    inline uint16_t getPowerOnVoltage() const {
        return powerOnVoltage;
    }
protected:
    bool init() override;
    bool fetchVoltageBattery() override;
    bool fetchCurrentBattery() override;
    bool fetchVoltageSolar() override;
    bool fetchCurrentSolar() override;

private:
    bool fetchOthersData();

    bool _isNight = false;
    uint16_t status = 0, powerOffVoltage = 0, powerOnVoltage = 0;
    mpptChg *charger;
};

#endif //RP2040_LORA_APRS_ENERGYMPPTCHG_H
