#include "Threads/Energy/EnergyMpptChg.h"
#include "ArduinoLog.h"
#include "System.h"
#include "variant.h"

EnergyMpptChg::EnergyMpptChg(System *system) : EnergyThread(system, PSTR("ENERGY_MPPTCHG")) {
#ifdef USE_MPPTCHG
    charger = &system->mpptChgCharger;
#else
    assert(charger != nullptr);
#endif
}

bool EnergyMpptChg::init() {
    return charger->begin()
    && charger->getConfigurationValue(CFG_PWR_OFF_TH, &powerOffVoltage)
    && charger->getConfigurationValue(CFG_PWR_ON_TH, &powerOnVoltage);
}

void EnergyMpptChg::run() {
    EnergyThread::run();
    fetchOthersData();
}

bool EnergyMpptChg::fetchVoltageBattery() {
    return charger->getIndexedValue(VAL_VB, &vb);
}

bool EnergyMpptChg::fetchCurrentBattery() {
    return charger->getIndexedValue(VAL_IB, &ib);
}

bool EnergyMpptChg::fetchVoltageSolar() {
    return charger->getIndexedValue(VAL_VS, &vs);
}

bool EnergyMpptChg::fetchCurrentSolar() {
    return charger->getIndexedValue(VAL_IS, &is);
}

bool EnergyMpptChg::fetchOthersData() {
    return charger->isNight(&_isNight) && charger->getStatusValue(SYS_STATUS, &status);
}

bool EnergyMpptChg::setPowerOnOff(uint16_t powerOnVoltage, uint16_t powerOffVoltage) {
    Log.infoln(F("[ENERGY_MPPTCHG] Power On : %dmV and Power Off : %dmV"), powerOnVoltage, powerOffVoltage);

    this->powerOnVoltage = powerOnVoltage;
    this->powerOffVoltage = powerOffVoltage;

    return charger->setConfigurationValue(CFG_PWR_ON_TH, powerOnVoltage)
           && charger->setConfigurationValue(CFG_PWR_OFF_TH, powerOffVoltage);
}