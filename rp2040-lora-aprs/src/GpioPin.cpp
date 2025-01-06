#include "GpioPin.h"
#include "ArduinoLog.h"

GpioPin::GpioPin(pin_size_t pin, PinMode mode, bool isAdc, bool inverted, bool state) : pin(pin), mode(mode),
    isAdc(isAdc), inverted(inverted) {
    pinMode(pin, mode);

    if (mode == OUTPUT) {
        setState(state);
    }
}

void GpioPin::setState(bool state) {
    assert(mode == OUTPUT);

    if (pin != LED_BUILTIN) {
        Log.infoln(F("[GPIO_%d] Set %T"), pin, state);
    }

    digitalWrite(pin, inverted == !state);

    currentState = state;
}

bool GpioPin::getState() {
    if (mode == OUTPUT) {
        return currentState;
    }

    assert(!isAdc);

    bool result = digitalRead(pin);

    if (inverted) {
        result = !result;
    }

    Log.traceln(F("[GPIO_%d] Get currentState %T"), pin, result);

    return result;
}

uint16_t GpioPin::getValue() {
    assert(mode != OUTPUT);
    assert(isAdc);

    uint16_t value = analogRead(pin);

    Log.traceln(F("[GPIO_%d] Get value %d"), pin, value);

    return value;
}