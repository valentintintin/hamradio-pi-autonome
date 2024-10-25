#include "Threads/Send/SendPositionThread.h"
#include "System.h"
#include "ArduinoLog.h"

SendPositionThread::SendPositionThread(System *system) : SendThread(system, INTERVAL_POSITION_AND_WEATHER_APRS, PSTR("SEND_POSITION")) {
}

bool SendPositionThread::send() {
    return system->communication.sendPosition(APRS_COMMENT);
}
