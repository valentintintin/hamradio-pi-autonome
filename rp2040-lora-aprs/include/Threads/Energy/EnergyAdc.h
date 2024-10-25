#ifndef RP2040_LORA_APRS_ENERGYADC_H
#define RP2040_LORA_APRS_ENERGYADC_H

#include "Threads/EnergyThread.h"
#include "GpioPin.h"

#define BATTERY_SENSE_SAMPLES 15
//  ratio of voltage divider = 3.0 (R17=200k, R18=100k)
#define BATTERY_SENSE_RESOLUTION_BITS ADC_RESOLUTION
#define ADC_MULTIPLIER 3.1 // 3.0 + a bit for being optimistic
#define AREF_VOLTAGE 3.3

class EnergyAdc : public EnergyThread {
public:
    explicit EnergyAdc(System* system, GpioPin *adc);
protected:
    bool fetchVoltageBattery() override;
private:
    GpioPin *adc;
};


#endif //RP2040_LORA_APRS_ENERGYADC_H
