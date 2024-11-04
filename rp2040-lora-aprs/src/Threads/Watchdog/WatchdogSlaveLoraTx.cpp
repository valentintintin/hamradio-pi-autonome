#include "Threads/Watchdog/WatchdogSlaveLoraTx.h"

#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"

WatchdogSlaveLoraTx::WatchdogSlaveLoraTx(System *system) : WatchdogThread(system, WATCHDOG_TIME_LORA_TX_OUT, PSTR("WATCHDOG_LORA_TX")) {
}

bool WatchdogSlaveLoraTx::runOnce() {
    if (millis() < WATCHDOG_TIME_AFTER_BOOT) {
        return true;
    }

    if (hasTx) {
        hasTx = false;
        Log.traceln(F("[WATCHDOG_LORA_TX] Dog fed"));
        return true;
    }

    Log.errorln(F("[WATCHDOG_LORA_TX] No TX for a long time, reboot"));
    delayWdt(1000);
    rp2040.reboot();
    return true;
}

bool WatchdogSlaveLoraTx::feed() {
    Log.infoln(F("[WATCHDOG_LORA_TX] Feed dog"));
    hasTx = true;
    return true;
}