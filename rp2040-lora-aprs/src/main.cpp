#include "utils.h"
#include "System.h"

char bufferText[BUFFER_LENGTH]{};

System systemControl;

void setup() {
    Serial.begin(115200);

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
        if (milliseconds - elapsedTime >= 5000) {
            delay(5000);
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