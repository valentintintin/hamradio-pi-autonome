#include <radio/radio.h>
#include "ArduinoLog.h"
#include <EEPROM.h>
#include "innerWdt.h"
#include "System.h"

#ifdef USE_SCREEN
System::System(SH1107Wire *display, CubeCell_NeoPixel *pixels, TimerEvent_t *wakeUpEvent) :
        display(display), pixels(pixels), wakeUpEvent(wakeUpEvent),
        mpptMonitor(this, WireUsed), weatherSensors(this), gpio(this), command(this) {
}
#else
System::System(CubeCell_NeoPixel *pixels, TimerEvent_t *wakeUpEvent) :
        pixels(pixels), RTC(Wire), wakeUpEvent(wakeUpEvent),
        mpptMonitor(this, Wire), weatherSensors(this), gpio(this), command(this) {
}
#endif

bool System::begin(RadioEvents_t *radioEvents) {
    Log.infoln(F("[SYSTEM] Starting"));
    printJsonSystem(PSTR("Starting"));

    Wire.begin();

    pixels->begin();
    pixels->clear();

#ifdef USE_SCREEN
    if (!display->init()) {
        serialError(PSTR("[SYSTEM] Display error"), false);
    }
#endif

    turnScreenOn();

    turnOnRGB(COLOR_CYAN);

    EEPROM.begin(512);

    if (EEPROM.read(EEPROM_ADDRESS_VERSION) != EEPROM_VERSION) {
        setFunctionAllowed(EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY, functionsAllowed[EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_DIGIPEATER, functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_TELEMETRY, functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY]);
        setFunctionAllowed(EEPROM_ADDRESS_APRS_POSITION, functionsAllowed[EEPROM_ADDRESS_APRS_POSITION]);
        setFunctionAllowed(EEPROM_ADDRESS_SLEEP, functionsAllowed[EEPROM_ADDRESS_SLEEP]);
        setFunctionAllowed(EEPROM_ADDRESS_RESET_ON_ERROR, functionsAllowed[EEPROM_ADDRESS_RESET_ON_ERROR]);
        setFunctionAllowed(EEPROM_ADDRESS_WATCHDOG, functionsAllowed[EEPROM_ADDRESS_WATCHDOG]);
        setFunctionAllowed(EEPROM_ADDRESS_WATCHDOG_LORA_TX, functionsAllowed[EEPROM_ADDRESS_WATCHDOG_LORA_TX]);

        EEPROM.write(EEPROM_ADDRESS_VERSION, EEPROM_VERSION);
        EEPROM.commit();
    } else {
        functionsAllowed[EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY] = EEPROM.read(EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY);
        functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER] = EEPROM.read(EEPROM_ADDRESS_APRS_DIGIPEATER);
        functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY] = EEPROM.read(EEPROM_ADDRESS_APRS_TELEMETRY);
        functionsAllowed[EEPROM_ADDRESS_APRS_POSITION] = EEPROM.read(EEPROM_ADDRESS_APRS_POSITION);
        functionsAllowed[EEPROM_ADDRESS_SLEEP] = EEPROM.read(EEPROM_ADDRESS_SLEEP);
        functionsAllowed[EEPROM_ADDRESS_RESET_ON_ERROR] = EEPROM.read(EEPROM_ADDRESS_RESET_ON_ERROR);
        functionsAllowed[EEPROM_ADDRESS_WATCHDOG] = EEPROM.read(EEPROM_ADDRESS_WATCHDOG);
        functionsAllowed[EEPROM_ADDRESS_WATCHDOG_LORA_TX] = EEPROM.read(EEPROM_ADDRESS_WATCHDOG_LORA_TX);
    }

    sprintf_P(bufferText, PSTR("MPPT Watchdog safety: %d Aprs Digi : %d Telem : %d Position : %d Sleep : %d Reset : %d Watchdog : %d Watchdog LoRa TX : %d"),
            functionsAllowed[EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY],
            functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER],
            functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY],
            functionsAllowed[EEPROM_ADDRESS_APRS_POSITION],
            functionsAllowed[EEPROM_ADDRESS_SLEEP],
            functionsAllowed[EEPROM_ADDRESS_RESET_ON_ERROR],
            functionsAllowed[EEPROM_ADDRESS_WATCHDOG],
            functionsAllowed[EEPROM_ADDRESS_WATCHDOG_LORA_TX]
    );

    Log.infoln(F("[SYSTEM] EEPROM %s"), bufferText);
    displayText(PSTR("EEPROM"), bufferText, 3000);

    weatherSensors.begin();
    mpptMonitor.begin();
    communication->begin(radioEvents);

    if (USE_RTC) {
        if (SET_RTC > 0 && RTClib::now().year() < CURRENT_YEAR) {
            Log.warningln(F("[TIME] Set clock to %u"), SET_RTC + 60);
            RTC.setEpoch(SET_RTC + 60);
        }

        setTimeFromRTcToInternalRtc(RTClib::now().unixtime());

        nowToString(bufferText);
        Log.infoln(F("[SYSTEM] Started at %s"), bufferText);
    } else {
        Log.infoln(F("[SYSTEM] Started"));
    }

    printJsonSystem(PSTR("started"));
    displayText(PSTR("Started"), bufferText);

    return true;
}

void System::update() {
    Stream *streamReceived = nullptr;

    if (SerialPi->available()) {
        streamReceived = SerialPi;
        Log.verboseln(F("Serial Pi incoming"));
    } else if (Serial.available()) {
        streamReceived = &Serial;
        Log.verboseln(F("Serial USB incoming"));
    }

    if (streamReceived != nullptr) {
        size_t lineLength = streamReceived->readBytesUntil('\n', bufferText, 150);
        bufferText[lineLength] = '\0';

        Log.traceln(PSTR("[SERIAL] Received %s"), bufferText);

        command.processCommand(bufferText);

        streamReceived->flush();
    } else {
        if (timerBoxOpened.hasExpired() && isBoxOpened() && millis() >= 600000) { // 10 minutes
            Log.warningln(F("[SYSTEM] Box opened !"));
            printJsonSystem(PSTR("alert"));

            timerBoxOpened.restart();
            communication->sendMessage(PSTR(APRS_DESTINATION), PSTR("Box opened !"));
        }

        if (forceSendTelemetry || timerState.hasExpired()) {
            printJsonSystem(PSTR("running"));

            timerState.restart();
        }

        mpptMonitor.update();
        weatherSensors.update();

        if (!mpptMonitor.isPowerEnabled()) {
            if (gpio.isRelay2Enabled()) {
                gpio.setRelay2(false);
            }

            if (gpio.isRelay1Enabled()) {
                gpio.setRelay1(false);
            }

            if (isFunctionAllowed(EEPROM_ADDRESS_SLEEP) && mpptMonitor.getVoltageBattery() < VOLTAGE_TO_SLEEP && millis() >= DURATION_TO_WAIT_BEFORE_SLEEP) {
                Log.infoln(F("[SLEEP] Battery is %dV and under of %dV so sleep"), mpptMonitor.getVoltageBattery(), VOLTAGE_TO_SLEEP);
                displayText(PSTR("Sleep"), PSTR("Battery too low so sleep"), 1000);
                sleep(SLEEP_DURATION);
            }
        }

        if (timerTelemetry.hasExpired() || timerPosition.hasExpired() || timerStatus.hasExpired()
            || forceSendTelemetry || forceSendPosition
        ) {
            communication->update(forceSendTelemetry || timerTelemetry.hasExpired(),
                                  forceSendPosition || timerPosition.hasExpired(),
                                  timerStatus.hasExpired());
            forceSendTelemetry = false;
            forceSendPosition = false;

            if (timerTelemetry.hasExpired()) {
                timerTelemetry.restart();
            }

            if (timerPosition.hasExpired()) {
                timerPosition.restart();
            }

            if (timerStatus.hasExpired()) {
                timerStatus.restart();
            }
        }
    }

    if (timerScreen.hasExpired()) {
        turnScreenOff();
    }

    if (timerBlinker.hasExpired() && ledColor == 0) {
        turnOnRGB(COLOR_ORANGE);
        turnOffRGB();
    }

    if (isFunctionAllowed(EEPROM_ADDRESS_WATCHDOG_LORA_TX) && communication->hasWatchdogLoraTxExpired()) {
        serialError(PSTR("[LORA_TX_WATCHDOG] No TX for a long time, reboot"), false);
        displayText("LoRa TX Watchdog", "No TX, reboot");
        delayWdt(1000);
        CySoftwareReset();
    }

    delay(10);
}

void System::turnOnRGB(uint32_t color) {
    if (color == ledColor) {
        return;
    }

    ledColor = color;

    Log.traceln(F("[LED] Turn led color : 0x%x"), color);

    uint8_t red, green, blue;
    red = (uint8_t) (color >> 16);
    green = (uint8_t) (color >> 8);
    blue = (uint8_t) color;
    pixels->setPixelColor(0, CubeCell_NeoPixel::Color(red, green, blue));
    pixels->show();

    System::delayWdt(250);
    timerBlinker.restart();
}

void System::turnOffRGB() {
    turnOnRGB(0);
}

void System::turnScreenOn() {
    if (!screenOn) {
        screenOn = true;
        Log.traceln(F("[SCREEN/LED] Turn screen/led on"));

#ifdef USE_SCREEN
        display->wakeup();
#endif

        timerScreen.restart();
    }
}

void System::turnScreenOff() {
    if (screenOn) {
        Log.traceln(F("[SCREEN/LED] Turn screen/led off"));
#ifdef USE_SCREEN
        display->sleep();
#endif

        screenOn = false;
    }

    turnOffRGB();
}

void System::displayText(const char *title, const char *content, uint16_t pause) const {
#ifdef USE_SCREEN
    if (!screenOn) {
        return;
    }

    Log.traceln(F("[Display] %s --> %s"), title, content);
    display->clear();
    display->drawString(0, 0, title);
    display->drawStringMaxWidth(0, 10, 120, content);
    display->display();
    System::delayWdt(pause);
#endif
}

DateTime System::nowToString(char *result) {
    DateTime now = RTClib::now();
    if (abs(CURRENT_YEAR - now.year()) >= 2) {
        sprintf(result, "%04d-%02d-%02dT%02d:%02d:%02d Uptime %lds", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), millis() / 1000);
        Log.warningln("[TIME] Use System instead of RTC. RTC date : %s", result);
        now = DateTime(TimerGetSysTime().Seconds);
    }
    sprintf(result, "%04d-%02d-%02dT%02d:%02d:%02d Uptime %lds", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), millis() / 1000);
    return now;
}

void System::setTimeFromRTcToInternalRtc(uint64_t epoch) {
    TimerSysTime_t currentTime;
    currentTime.Seconds = epoch;
    TimerSetSysTime(currentTime);
}

bool System::isBoxOpened() const {
    return millis() >= 60000 && gpio.getLdr() >= LDR_ALARM_LEVEL;
}

void System::serialError(const char *content, bool addError) {
    turnOnRGB(COLOR_VIOLET);
    Log.errorln(content);
    if (addError) {
        this->addError();
    }
    printJsonSystem((char*)content);
    if (screenOn) {
        delayWdt(2000);
    }
    turnOffRGB();
}

void System::setFunctionAllowed(byte function, bool allowed) {
    sprintf_P(bufferText, PSTR("[EEPROM] Set %d to %d"), function, allowed);
    Log.infoln(bufferText);
    displayText("EEPROM", bufferText);

    functionsAllowed[function] = allowed;

    EEPROM.write(function, allowed);
    EEPROM.commit();

    timerState.setExpired();

    if (function == EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY && allowed) {
        mpptMonitor.resetWatchdogSafety();
    } else if (function == EEPROM_ADDRESS_RESET_ON_ERROR) {
        nbError = 0;
    } else if (function == EEPROM_ADDRESS_WATCHDOG) {
        if (allowed) {
            Log.infoln(F("Enable internal watchdog"));
            innerWdtEnable(true);
        } else {
            Log.infoln(F("Disable internal watchdog"));
            wdt_isr_Disable();

        }
    } else if (function == EEPROM_ADDRESS_WATCHDOG_LORA_TX) {
      communication->resetWatchdogLoraTx();
    }
}

void System::printJsonSystem(const char *state) {
    DateTime now = nowToString(bufferText);

    serialJsonWriter
            .beginObject()
            .property(F("type"), PSTR("system"))
            .property(F("state"), (char*)state)
            .property(F("boxOpened"), isBoxOpened())
            .property(F("temperatureRtc"), RTC.getTemperature())
            .property(F("time"), now.unixtime())
            .property(F("uptime"), millis() / 1000)
            .property(F("nbError"), nbError)
            .property(F("mpptWatchdogSafetyTimer"), mpptMonitor.getWatchdogSafetyTimeLeft() / 1000)
            .property(F("mpptWatchdogSafety"), functionsAllowed[EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY])
            .property(F("aprsDigipeater"), functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER])
            .property(F("aprsTelemetry"), functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY])
            .property(F("aprsPosition"), functionsAllowed[EEPROM_ADDRESS_APRS_POSITION])
            .property(F("sleep"), functionsAllowed[EEPROM_ADDRESS_SLEEP])
            .property(F("resetError"), functionsAllowed[EEPROM_ADDRESS_RESET_ON_ERROR])
            .property(F("watchdog"), functionsAllowed[EEPROM_ADDRESS_WATCHDOG])
            .property(F("watchdogLoraTx"), functionsAllowed[EEPROM_ADDRESS_WATCHDOG_LORA_TX])
            .property(F("watchdogLoraTxTimer"), communication->getWatchdogLoraTxTimeLeft() / 1000)
            .endObject(); SerialPi->println();

    gpio.printJson();

    Log.infoln(F("[TIME] %s. Temperature RTC: %FC"), bufferText, RTC.getTemperature());
    displayText(PSTR("Time"), bufferText, 1000);

    Log.infoln(F("[SYSTEM] EEPROM MPPT Watchdog safety: %T Aprs Digi : %T Telem : %T Position : %T Sleep : %T Reset : %T Watchdog : %T Watchdog LoRa TX : %T"),
               functionsAllowed[EEPROM_ADDRESS_MPPT_WATCHDOG_SAFETY],
               functionsAllowed[EEPROM_ADDRESS_APRS_DIGIPEATER],
               functionsAllowed[EEPROM_ADDRESS_APRS_TELEMETRY],
               functionsAllowed[EEPROM_ADDRESS_APRS_POSITION],
               functionsAllowed[EEPROM_ADDRESS_SLEEP],
               functionsAllowed[EEPROM_ADDRESS_RESET_ON_ERROR],
               functionsAllowed[EEPROM_ADDRESS_WATCHDOG],
               functionsAllowed[EEPROM_ADDRESS_WATCHDOG_LORA_TX]
    );
}

void System::sleep(uint64_t time) {
    turnOnRGB(COLOR_BLUE);

    sprintf_P(bufferText, PSTR("[SLEEP] Sleep during %ums"), time);
    Log.infoln(bufferText);
    displayText("Sleep", bufferText);

    gpio.setRelay2(false);
    gpio.setRelay1(false);
    mpptMonitor.setWatchdog(SLEEP_DURATION + DURATION_TO_WAIT_BEFORE_SLEEP + 10000, 1);

    turnScreenOff();
    digitalWrite(Vext, HIGH); // 0V
    Radio.Sleep();
    TimerSetValue(wakeUpEvent, time);
    TimerStart(wakeUpEvent);

    if (CySysWdtGetEnabledStatus()) {
        wdt_isr_Disable();
    }

    lowPowerHandler();
}

void System::resetTimerJson() {
    timerState.hasExpired();
    mpptMonitor.update(true);
    weatherSensors.update(true);
}

void System::addError() {
    nbError++;

    Log.infoln(F("[SYSTEM] Add error : %d"), nbError);

    if (nbError >= MAX_ERROR_TO_RESET && isFunctionAllowed(EEPROM_ADDRESS_RESET_ON_ERROR)) {
        Log.warningln(F("[SYSTEM] Nb error too high so we reboot"));
        System::delayWdt(500);
        CySoftwareReset();
    }
}

void System::removeOneError() {
    if (nbError > 0) {
        nbError--;
        Log.traceln(F("[SYSTEM] Remove an error: %d"), nbError);
    }
}

void System::delayWdt(uint32_t milliseconds) {
    if (CySysWdtGetEnabledStatus()) {
        unsigned long startTime = millis();
        unsigned long elapsedTime = 0;

        while (elapsedTime < milliseconds) {
            if (milliseconds - elapsedTime >= 1000) {
                delay(1000);
                feedInnerWdt();
            } else {
                delay(milliseconds - elapsedTime);
            }
            elapsedTime = millis() - startTime;
        }
    } else {
        delay(milliseconds);
    }
}

//void EEPROM_writeInt(int address, int value) {
//    byte lowByte = value & 0xFF; // low part
//    byte highByte = (value >> 8) & 0xFF;
//
//    EEPROM.write(address, lowByte);
//    EEPROM.write(address + 1, highByte);
//    EEPROM.commit();
//}
//
//int EEPROM_readInt(int address) {
//    byte lowByte = EEPROM.read(address);
//    byte highByte = EEPROM.read(address + 1);
//
//    return (highByte << 8) | lowByte;
//}