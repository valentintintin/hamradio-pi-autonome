#include "Threads/Energy/EnergyDummy.h"

EnergyDummy::EnergyDummy(System *system) : EnergyThread(system, PSTR("ENERGY_DUMMY")) {}

bool EnergyDummy::init() {
    // Init a random ?
    return true;
}

bool EnergyDummy::fetchVoltageBattery() {
    vb = 12345;
    return true;
}

bool EnergyDummy::fetchCurrentBattery() {
    vb = 123;
    return true;
}

bool EnergyDummy::fetchVoltageSolar() {
    vb = 23456;
    return true;
}

bool EnergyDummy::fetchCurrentSolar() {
    vb = 345;
    return true;
}