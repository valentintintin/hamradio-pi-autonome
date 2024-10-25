#include "Threads/Send/SendTelemetriesThread.h"
#include "System.h"
#include "ArduinoLog.h"

SendTelemetriesThread::SendTelemetriesThread(System *system) : SendThread(system, INTERVAL_TELEMETRY_APRS, PSTR("SEND_TELEMETRIES")) {
}

bool SendTelemetriesThread::send() {
    return system->communication.sendTelemetry();
}
