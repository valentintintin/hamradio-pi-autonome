#include "I2CSlave.h"
#include "variant.h"
#include "System.h"

#include "ArduinoLog.h"

uint8_t I2CSlave::currentRegToRead = 0;
System *I2CSlave::system = nullptr;

void I2CSlave::begin(System *system) {
    I2CSlave::system = system;

    Log.infoln(F("[I2C_SLAVE] Begin"));
    Wire1.begin(I2C_SLAVE_ADDRESS);
    Wire1.onRequest(onRequest);
    Wire1.onReceive(onReceive);
}

void I2CSlave::onRequest() {
    uint32_t value = 0;

    switch (currentRegToRead) {
        case REG_PING:
#if defined(USE_ENERGY_DUMMY) || defined(USE_MPPTCHG) || defined(USE_INA3221) || defined(USE_BATTERY_ADC)
    value += HAS_POWER;
#endif
#ifdef USE_WEATHER
    value += HAS_WEATHER;
#endif
            break;
#if defined(USE_ENERGY_DUMMY) || defined(USE_MPPTCHG) || defined(USE_INA3221) || defined(USE_BATTERY_ADC)
        case REG_BATTERY_VOLTAGE:
            value = I2CSlave::system->energyThread->getVoltageBattery();
            break;
        case REG_BATTERY_CURRENT:
            value = I2CSlave::system->energyThread->getCurrentBattery();
            break;
        case REG_SOLAR_VOLTAGE:
            value = I2CSlave::system->energyThread->getVoltageSolar();
            break;
        case REG_SOLAR_CURRENT:
            value = I2CSlave::system->energyThread->getCurrentSolar();
            break;
#endif
#ifdef USE_WEATHER
        case REG_TEMPERATURE:
            value = (int16_t) I2CSlave::system->weatherThread.getTemperature() * 100;
            break;
        case REG_PRESSURE:
            value = (int16_t) I2CSlave::system->weatherThread.getPressure();
            break;
        case REG_HUMIDITY:
            value = (int16_t) I2CSlave::system->weatherThread.getHumidity();
            break;
#endif
    }

    Log.traceln(F("[I2C_SLAVE] Send %d for register %d"), value, currentRegToRead);

    Wire1.write((value >> 8) & 0xFF);
    Wire1.write(value & 0xFF);
}

void I2CSlave::onReceive(int bytes) {
    Log.traceln(F("[I2C_SLAVE] Receive %d bytes"), bytes);

    if (bytes == 1) {
        currentRegToRead = Wire.read();
        Log.traceln(F("[I2C_SLAVE] Receive value %d to read"), currentRegToRead);
    }
}