#include "GpioPin.h"
#include "ArduinoLog.h"

GpioPin::GpioPin(pin_size_t pin, PinMode mode, bool isAdc, bool inverted) : pin(pin), mode(mode),
    isAdc(isAdc), inverted(inverted) {
    pinMode(pin, mode);
}

void GpioPin::setState(bool state) {
    assert(mode == OUTPUT);

    Log.infoln(F("[GPIO_%d] Set %d"), pin, state);

    digitalWrite(pin, inverted == !state);
}

bool GpioPin::getState() {
    assert(mode != OUTPUT);
    assert(!isAdc);

    bool result = digitalRead(pin);

    if (inverted) {
        result = !result;
    }

    Log.infoln(F("[GPIO_%d] Get state %t"), pin, result);

    return result;
}

uint16_t GpioPin::getValue() {
    assert(mode != OUTPUT);
    assert(isAdc);

    uint16_t value = analogRead(pin);

    Log.infoln(F("[GPIO_%d] Get value %d"), pin, value);

    return value;
}