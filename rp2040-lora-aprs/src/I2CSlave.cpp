#include <hardware/rtc.h>
#include "I2CSlave.h"
#include "variant.h"
#include "System.h"

#include "ArduinoLog.h"

uint8_t I2CSlave::currentRegToRead = 0;
System *I2CSlave::system = nullptr;

void I2CSlave::begin(System *system) {
    I2CSlave::system = system;

    if (!Wire1.setSDA(2)) {
        Log.errorln(F("[I2C_SLAVE] Begin KO. Wrong SDA pin"));
        return;
    }

    if (!Wire1.setSCL(3)) {
        Log.errorln(F("[I2C_SLAVE] Begin KO. Wrong SCL pin"));
        return;
    }

    Wire1.begin(I2C_SLAVE_ADDRESS);
    Wire1.onRequest(onRequest);
    Wire1.onReceive(onReceive);

    Log.infoln(F("[I2C_SLAVE] Begin on address %x and pin SDA 2, SCL 3"), I2C_SLAVE_ADDRESS);
}

void I2CSlave::onRequest() {
    uint32_t value = 0;
#ifdef USE_RTC
    datetime_t datetime;
    rtc_get_datetime(&datetime);
#endif

    switch (currentRegToRead) {
        case REG_PING:
#if defined(USE_ENERGY_DUMMY) || defined(USE_MPPTCHG) || defined(USE_INA3221) || defined(USE_BATTERY_ADC)
    value += HAS_POWER;
#endif
#ifdef USE_WEATHER
    value += HAS_WEATHER;
#endif
#ifdef USE_RTC
    value += HAS_RTC;
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
            value = (int16_t) (I2CSlave::system->weatherThread.getTemperature() * 100.0);
            break;
        case REG_PRESSURE:
            value = (int16_t) I2CSlave::system->weatherThread.getPressure();
            break;
        case REG_HUMIDITY:
            value = (int16_t) I2CSlave::system->weatherThread.getHumidity();
            break;
#endif
#ifdef USE_RTC
        case REG_SECONDS:
            value = (uint8_t) datetime.sec;
            break;
        case REG_MINUTES:
            value = (uint8_t) datetime.min;
            break;
        case REG_HOURS:
            value = (uint8_t) datetime.hour;
            break;
        case REG_DAYS:
            value = (uint8_t) datetime.day;
            break;
        case REG_MONTHS:
            value = (uint8_t) datetime.month;
            break;
        case REG_YEARS:
            value = datetime.year + 1900;
            break;
#endif
        default:
            Log.warningln(F("[I2C_SLAVE] Register %x not found"), currentRegToRead);
            break;
    }

    Log.infoln(F("[I2C_SLAVE] Send %d for register %x"), value, currentRegToRead);

    Wire1.write((value >> 8) & 0xFF);
    Wire1.write(value & 0xFF);
}


void I2CSlave::onReceive(int bytes) {
    Log.traceln(F("[I2C_SLAVE] Receive %d bytes"), bytes);

    if (bytes == 1) {
        currentRegToRead = Wire1.read();
        Log.traceln(F("[I2C_SLAVE] Receive value %x to read"), currentRegToRead);

#ifdef USE_WATCHDOG_MESHTASTIC
        system->watchdogMeshtastic->feed();
#endif
    }
}