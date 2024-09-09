#include "ArduinoLog.h"
#include "System.h"
#include "MpptMonitor.h"
#include <EEPROM.h>

MpptMonitor::MpptMonitor(System *system, TwoWire &wire) : system(system), wire(wire) {
}

bool MpptMonitor::begin() {
    if (!charger.begin(wire)) {
        system->displayText(PSTR("Mppt error"), PSTR("Init failed"));
        system->serialError(PSTR("[MPPT] Charger error"));

        return false;
    }

    init = true;
    useWatchdogSafety = true;

    return true;
}

bool MpptMonitor::update(bool force) {
    if (!force && !timer.hasExpired()) {
        return false;
    }

    timer.restart();

    if (!init && !begin()) {
        return false;
    }

    bool hasImportantChange = false;
    bool oldBoolValue;

    Log.traceln(F("[MPPT] Fetch charger data"));

    if (!charger.getStatusValue(SYS_STATUS, &status)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data status"));
        system->serialError(PSTR("[MPPT] Fetch charger data status error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger status : %s"), mpptChg::getStatusAsString(status));
    }

    if (!charger.getIndexedValue(VAL_VS, &vs)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data VS"));
        system->serialError(PSTR("[MPPT] Fetch charger data VS error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger VS : %d"), vs);
    }

    if (!charger.getIndexedValue(VAL_IS, &is)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data IS"));
        system->serialError(PSTR("[MPPT] Fetch charger data IS error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger IS : %d"), is);
    }

    if (!charger.getIndexedValue(VAL_VB, &vb)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data VB"));
        system->serialError(PSTR("[MPPT] Fetch charger data VB error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger VB : %d"), vb);
    }

    if (!charger.getIndexedValue(VAL_IB, &ib)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data IB"));
        system->serialError(PSTR("[MPPT] Fetch charger data IB error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger IB : %d"), ib);
    }

    oldBoolValue = night;
    if (!charger.isNight(&night)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data night"));
        system->serialError(PSTR("[MPPT] Fetch charger data night error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger night : %T"), night);
        hasImportantChange |= oldBoolValue != night;
    }

    oldBoolValue = alert;
    if (!charger.isAlert(&alert)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data alert"));
        system->serialError(PSTR("[MPPT] Fetch charger data alert error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger alert : %T"), alert);
        hasImportantChange |= oldBoolValue != alert;
    }

    oldBoolValue = powerEnabled;
    if (!charger.isPowerEnabled(&powerEnabled)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power enabled"));
        system->serialError(PSTR("[MPPT] Fetch charger data power enabled error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger power enabled : %T"), powerEnabled);
        hasImportantChange |= oldBoolValue != powerEnabled;
    }

    oldBoolValue = watchdogEnabled;
    if (!charger.getWatchdogEnable(&watchdogEnabled)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog"));
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger watchdog : %T"), watchdogEnabled);
        hasImportantChange |= oldBoolValue != watchdogEnabled;
    }

    uint16_t oldIntValue = watchdogPowerOffTime;
    if (!charger.getWatchdogPoweroff(&watchdogPowerOffTime)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog poweroff"));
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog poweroff error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger watchdog poweroff : %d"), watchdogPowerOffTime);
        hasImportantChange |= oldIntValue != watchdogPowerOffTime;
    }

    if (!charger.getWatchdogTimeout(&watchdogCounter)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog counter"));
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog counter error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger watchdog counter : %d"), watchdogCounter);
    }

    if (!charger.getConfigurationValue(CFG_PWR_OFF_TH, &powerOffVoltage)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power off voltage"));
        system->serialError(PSTR("[MPPT] Fetch charger data power off voltage error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger power off voltage : %d"), powerOffVoltage);
    }

    if (!charger.getConfigurationValue(CFG_PWR_ON_TH, &powerOnVoltage)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data power on voltage"));
        system->serialError(PSTR("[MPPT] Fetch charger data power on voltage error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger power on voltage : %d"), powerOnVoltage);
    }

    if (!charger.getIndexedValue(VAL_INT_TEMP, &temperature)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch temperature"));
        system->serialError(PSTR("[MPPT] Fetch charger data temperature error"));
        init = false;
        return false;
    } else {
        Log.verboseln(F("[MPPT] Charger temperature : %d"), temperature);
    }

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("mppt"))
            .property(F("batteryVoltage"), vb)
            .property(F("batteryCurrent"), ib)
            .property(F("solarVoltage"), vs)
            .property(F("solarCurrent"), is)
            .property(F("currentCharge"), getCurrentCharge())
            .property(F("temperature"), temperature / 10.0)
            .property(F("status"), status)
            .property(F("statusString"), (char*) mpptChg::getStatusAsString(status))
            .property(F("night"), night)
            .property(F("alert"), alert)
            .property(F("watchdogEnabled"), watchdogEnabled)
            .property(F("watchdogPowerOffTime"), watchdogPowerOffTime)
            .property(F("watchdogCounter"), watchdogCounter)
            .property(F("powerEnabled"), powerEnabled)
            .property(F("powerOffVoltage"), powerOffVoltage)
            .property(F("powerOnVoltage"), powerOnVoltage)
            .endObject(); SerialPi->println();

    Log.infoln(F("[MPPT] Vb: %dmV Ib: %dmA Vs: %dmV Is: %dmA Ic: %dmA Status: %s Night: %T Alert: %T WD: %T WDOff: %ds WDCnt: %ds 5V: %T PowOffVolt: %dV PowOnVolt: %dV Temperature: %DC TimerSafety: %ums"), vb, ib, vs, is, getCurrentCharge(), mpptChg::getStatusAsString(status), night, alert, watchdogEnabled, watchdogPowerOffTime, watchdogCounter, powerEnabled, powerOffVoltage, powerOnVoltage, temperature / 10.0f, timerWatchdogSafety.getTimeLeft());

    sprintf_P(bufferText, PSTR("Vb:%dmV Ib:%dmA Vs:%dmV Is:%dmA Ic:%dmA Status:%s PowOnVolt: %d"), vb, ib, vs, is, getCurrentCharge(), mpptChg::getStatusAsString(status), powerOnVoltage);
    system->displayText(PSTR("MPPT"), bufferText, 3000);

    sprintf_P(bufferText, PSTR("Night:%d Alert:%d WD:%d WDOff:%ds WDCnt:%ds 5V:%d PowOffVolt: %d TimerSafety: %ld"), night, alert, watchdogEnabled, watchdogPowerOffTime, watchdogCounter, powerEnabled, powerOffVoltage, timerWatchdogSafety.getTimeLeft());
    system->displayText(PSTR("MPPT"), bufferText, 3000);

    timer.restart();

    if (system->isFunctionAllowed(EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY)) {
        doWatchdogSafety();
    }

    if (hasImportantChange) {
        system->forceSendTelemetry = true;
    }

    system->removeOneError();

    return true;
}

bool MpptMonitor::setWatchdog(uint16_t powerOffTime, uint8_t timeoutTime) {
    if (!init) {
        if (!begin()) {
            return false;
        }
    }

    bool enabled = powerOffTime > 0;
    system->forceSendTelemetry = watchdogEnabled != enabled || powerOffTime != watchdogPowerOffTime;

    if (enabled) {
        if (!charger.setWatchdogPoweroff(powerOffTime)) {
            system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog poweroff"));
            system->serialError(PSTR("[MPPT_WATCHDOG]Change watchdog poweroff error"));
            init = false;
            system->forceSendTelemetry = false;
            return false;
        }

        watchdogPowerOffTime = powerOffTime;

        if (!charger.setWatchdogTimeout(timeoutTime)) {
            system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog counter"));
            system->serialError(PSTR("[MPPT_WATCHDOG]Change watchdog counter error"));
            init = false;
            system->forceSendTelemetry = false;
            return false;
        }

        watchdogCounter = timeoutTime;
    }

    if (!charger.setWatchdogEnable(enabled)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to set watchdog"));
        system->serialError(PSTR("[MPPT_WATCHDOG]Change watchdog enable error"));
        init = false;
        system->forceSendTelemetry = false;
        return false;
    }

    if (!charger.getWatchdogEnable(&watchdogEnabled)) {
        system->displayText(PSTR("Mttp error"), PSTR("Failed to fetch data watchdog"));
        system->serialError(PSTR("[MPPT] Fetch charger data watchdog error"));
        init = false;
        system->forceSendTelemetry = false;
        return false;
    }

    useWatchdogSafety = !watchdogEnabled;
    timerWatchdogSafety.restart();

    sprintf_P(bufferText, PSTR("[MPPT_WATCHDOG] Watchdog state : %d. PowerOff : %d, Counter : %d, TimerSafety : %ls"), enabled, powerOffTime, MPPT_WATCHDOG_TIMEOUT, timerWatchdogSafety.getTimeLeft());
    Log.infoln(bufferText);
    system->displayText(PSTR("WatchDog"), bufferText, 3000);

    timer.setExpired();
    system->removeOneError();

    return true;
}

bool MpptMonitor::setPowerOnOff(uint16_t powerOnVoltage, uint16_t powerOffVoltage) {
    sprintf_P(bufferText, PSTR("[MPPT_POWER] On : %dmV Off : %dmV"), powerOnVoltage, powerOffVoltage);
    Log.infoln(bufferText);
    system->displayText(PSTR("Power"), bufferText, 3000);

    return charger.setConfigurationValue(CFG_PWR_ON_TH, powerOnVoltage)
           && charger.setConfigurationValue(CFG_PWR_OFF_TH, powerOffVoltage);
}

void MpptMonitor::doWatchdogSafety() {
    if (alert) {
        useWatchdogSafety = false;
        return;
    }

    if (useWatchdogSafety && powerEnabled && timerWatchdogSafety.hasExpired()) {
        system->serialError(PSTR("[MPPT_WATCHDOG] Watchdog safety triggered ! Restart"));
        system->displayText(PSTR("Watchdog safety"), PSTR("Triggered so restart"));
        setWatchdog(10, 1);
    }
}

void MpptMonitor::resetWatchdogSafety() {
    timerWatchdogSafety.restart();
    useWatchdogSafety = true;

    Log.infoln(F("[MPPT_WATCHDOG] Reset safety timer to %ums"), timerWatchdogSafety.getTimeLeft());
}
