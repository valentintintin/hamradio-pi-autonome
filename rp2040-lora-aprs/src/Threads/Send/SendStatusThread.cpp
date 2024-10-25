#include "Threads/Send/SendStatusThread.h"
#include "ArduinoLog.h"
#include "System.h"

SendStatusThread::SendStatusThread(System *system) : SendThread(system, INTERVAL_STATUS_APRS, PSTR("SEND_STATUS")) {
}

bool SendStatusThread::send() {
    return system->communication.sendStatus(APRS_STATUS);
}