#ifndef RP2040_LORA_APRS_GPIOPIN_H
#define RP2040_LORA_APRS_GPIOPIN_H

#include "Arduino.h"

class GpioPin {
public:
    explicit GpioPin(pin_size_t pin, PinMode mode = OUTPUT, bool isAdc = false, bool inverted = false, bool state = false);
    void setState(bool state);
    bool getState();
    uint16_t getValue();

    inline pin_size_t getPin() const {
        return pin;
    }
private:
    pin_size_t pin;
    PinMode mode;
    bool currentState = false;
    bool inverted;
    bool isAdc;
};

#endif //RP2040_LORA_APRS_GPIOPIN_H
