#include "utils.h"
#include "System.h"

char bufferText[BUFFER_LENGTH]{};

System systemControl;

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);

    if (LOG_LEVEL != LOG_LEVEL_INFO) {
        delay(2500); // Wait for serial debug
    }

    Log.begin(LOG_LEVEL, &Serial);

    systemControl.begin();
}

void loop() {
    systemControl.loop();
}


void delayWdt(uint32_t milliseconds) {
#ifdef USE_WATCHDOG
    unsigned long startTime = millis();
    unsigned long elapsedTime = 0;

    while (elapsedTime < milliseconds) {
        if (milliseconds - elapsedTime >= 1000) {
            delay(1000);
            rp2040.wdt_reset();
        } else {
            delay(milliseconds - elapsedTime);
        }
        elapsedTime = millis() - startTime;
    }
#else
    delay(milliseconds);
#endif
}

void ledBlink(uint8_t howMany, uint16_t milliseconds) {
    for (uint8_t i = 0; i < howMany - 1; i++) {
        digitalWrite(PIN_LED, HIGH);
        delayWdt(milliseconds);
        digitalWrite(PIN_LED, LOW);
        delayWdt(milliseconds);
    }
}
