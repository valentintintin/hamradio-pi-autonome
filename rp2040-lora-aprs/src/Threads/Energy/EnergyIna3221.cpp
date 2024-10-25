#include "Threads/Energy/EnergyIna3221.h"

EnergyIna3221::EnergyIna3221(System *system, ina3221_ch_t channelBattery, ina3221_ch_t channelSolar) :
EnergyThread(system, PSTR("ENERGY_INA3221")), channelBattery(channelBattery), channelSolar(channelSolar) {}

bool EnergyIna3221::init() {
    ina3221.begin();
    ina3221.setShuntRes(100, 100, 100); // 0.1 Ohm shunt resistors

    return ina3221.getDieID() == 0x3220;
}

bool EnergyIna3221::fetchVoltageBattery() {
    vb = ina3221.getVoltage(channelBattery) * 1000;
    return true;
}

bool EnergyIna3221::fetchCurrentBattery() {
    vb = ina3221.getCurrent(channelBattery) * 1000;
    return true;
}

bool EnergyIna3221::fetchVoltageSolar() {
    vb = ina3221.getVoltage(channelSolar) * 1000;
    return true;
}

bool EnergyIna3221::fetchCurrentSolar() {
    vb = ina3221.getCurrent(channelSolar) * 1000;
    return true;
}