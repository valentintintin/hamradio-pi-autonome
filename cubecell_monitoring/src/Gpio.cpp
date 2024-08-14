
#include "Gpio.h"
#include "ArduinoLog.h"
#include "Config.h"
#include "System.h"

Gpio::Gpio(System *system) : system(system) {
    setWifi(wifi);
    setNpr(npr);
    setMeshtastic(meshtastic);
    initialized = true;
}

void Gpio::setState(uint8_t pin, bool enabled, const char* name, bool &status, bool inverted) {
    pinMode(pin, OUTPUT);

    if (inverted) {
        digitalWrite(pin, !enabled);
    } else {
        digitalWrite(pin, enabled);
    }

    status = enabled;

    if (initialized) {
        Log.infoln(F("[GPIO] %s (%d) change to state %T"), name, pin, status);

        printJson();
    }
}

void Gpio::setWifi(bool enabled) {
    setState(PIN_WIFI, enabled, PSTR("WIFI"), wifi, true);
}

void Gpio::setNpr(bool enabled) {
    if (npr != enabled) {
        system->forceSendTelemetry = true;
    }

    setState(PIN_NPR, enabled, PSTR("NPR"), npr, true);
}

uint16_t Gpio::getLdr() const {
//    return getAdcState(PIN_LDR, PSTR("LDR"));
    return getState(PIN_LDR, PSTR("LDR")) ? 0 : LDR_ALARM_LEVEL;
}

void Gpio::setMeshtastic(bool enabled) {
    setState(PIN_MESHTASTIC, enabled, PSTR("MESH"), meshtastic);
}

bool Gpio::getState(uint8_t pin, const char* name) const {
    pinMode(pin, INPUT);
    return digitalRead(pin);
}

uint16_t Gpio::getAdcState(uint8_t pin, const char *name) const {
    pinMode(pin, INPUT);
    return analogRead(pin);
}

void Gpio::printJson() const {
    Log.infoln(F("[GPIO] Wifi: %T NPR: %T Msh: %T Box LDR: %d"), isWifiEnabled(), isNprEnabled(), isMeshtasticEnabled(), getLdr());

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("gpio"))
            .property(F("wifi"), isWifiEnabled())
            .property(F("npr"), isNprEnabled())
            .property(F("msh"), isMeshtasticEnabled())
            .property(F("ldr"), getLdr())
            .endObject(); SerialPi->println();
}
