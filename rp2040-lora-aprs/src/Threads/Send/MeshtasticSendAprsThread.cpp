#include "Threads/Send/MeshtasticSendAprsThread.h"
#include "System.h"

MeshtasticSendAprsThread::MeshtasticSendAprsThread(System *system) : SendThread(system, system->settings.meshtastic.intervalSendItem, PSTR("SEND_MESHTASTIC_APRS"), system->settings.meshtastic.aprsSendItemEnabled) {}

bool MeshtasticSendAprsThread::runOnce() {
    return system->communication.sendItem(system->settings.meshtastic.itemName,
                                          system->settings.meshtastic.symbol,
                                          system->settings.meshtastic.symbolTable,
                                          system->settings.meshtastic.itemComment,
                                          system->watchdogMeshtastic->isFed() || force);
}

bool MeshtasticSendAprsThread::shouldRun(unsigned long time) {
    return SendThread::shouldRun(time) && system->watchdogMeshtastic->enabled;
}