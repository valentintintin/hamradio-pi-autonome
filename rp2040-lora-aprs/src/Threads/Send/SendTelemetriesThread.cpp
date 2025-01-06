#include "Threads/Send/SendTelemetriesThread.h"
#include "System.h"
#include "ArduinoLog.h"

SendTelemetriesThread::SendTelemetriesThread(System *system) : SendThread(system, system->settings.aprs.intervalTelemetry, PSTR("SEND_TELEMETRIES"), system->settings.aprs.telemetryEnabled) {
    force = true;
}

bool SendTelemetriesThread::runOnce() {
    return system->communication.sendTelemetry();
}

bool SendTelemetriesThread::shouldRun(unsigned long time) {
    return MyThread::shouldRun(time);
}
