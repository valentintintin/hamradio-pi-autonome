#include "Threads/Energy/EnergyAdc.h"
#include "ArduinoLog.h"

EnergyAdc::EnergyAdc(System *system, GpioPin *adc) : EnergyThread(system, PSTR("ENERGY_ADC")), adc(adc) {
}

bool EnergyAdc::fetchVoltageBattery() {
    uint32_t raw = 0;

    for (uint32_t i = 0; i < BATTERY_SENSE_SAMPLES; i++) {
        uint16_t value = adc->getValue();
        Log.verboseln(F("[ENERGY_ADC] Sample %d : %d"), i, value);
        raw += value;
    }

    vb = raw / BATTERY_SENSE_SAMPLES;
//   vb = ADC_MULTIPLIER * ((1000 * AREF_VOLTAGE) / pow(2, BATTERY_SENSE_RESOLUTION_BITS)) * raw

    return true;
}