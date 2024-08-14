#ifndef CUBECELL_MONITORING_SYSTEM_H
#define CUBECELL_MONITORING_SYSTEM_H

#include <cstdint>
#include <CubeCell_NeoPixel.h>
#include <DS3231.h>

#include "Communication.h"
#include "Timer.h"
#include "Config.h"
#include "Command.h"
#include "MpptMonitor.h"
#include "WeatherSensors.h"
#include "Gpio.h"

#ifdef USE_SCREEN
#include <HT_SH1107Wire.h>
#endif

class System {
public:
#ifdef USE_SCREEN
    explicit System(SH1107Wire *display, CubeCell_NeoPixel *pixels, TimerEvent_t *wakeUpEvent);
#else
    explicit System(CubeCell_NeoPixel *pixels, TimerEvent_t *wakeUpEvent);
#endif

    bool begin(RadioEvents_t *radioEvents);
    void update();
    void setTimeFromRTcToInternalRtc(uint64_t epoch);
    bool isBoxOpened() const;
    void setFunctionAllowed(byte function, bool allowed);
    void sleep(uint64_t time);
    void resetTimerJson();

    inline bool isFunctionAllowed(byte function) const {
        return functionsAllowed[function];
    }

    void turnOnRGB(uint32_t color);
    void turnOffRGB();
    void displayText(const char* title, const char* content, uint16_t pause = TIME_PAUSE_SCREEN) const;
    void serialError(const char* content, bool addError = true);
    void addError();
    void removeOneError();

    static DateTime nowToString(char *result);

    Communication *communication{};
    Command command;
    MpptMonitor mpptMonitor;
    WeatherSensors weatherSensors;
    Gpio gpio;
    DS3231 RTC;

    bool forceSendPosition = false;
    bool forceSendTelemetry = false;
    uint8_t nbError = 0;
private:
    bool screenOn = false;
    uint32_t ledColor = 0;

    /*
        EEPROM_ADDRESS_WATCHDOG_SAFETY
        EEPROM_ADDRESS_APRS_DIGIPEATER
        EEPROM_ADDRESS_APRS_TELEMETRY
        EEPROM_ADDRESS_APRS_POSITION
        EEPROM_ADDRESS_SLEEP
        EEPROM_ADDRESS_RESET_ON_ERROR
     */
    bool functionsAllowed[6] = {false, true, true, true, false, false};

    Timer timerStatus = Timer(INTERVAL_STATUS_APRS, false);
    Timer timerPosition = Timer(INTERVAL_POSITION_AND_WEATHER_APRS, true);
    Timer timerTelemetry = Timer(INTERVAL_TELEMETRY_APRS, false);
    Timer timerState = Timer(INTERVAL_STATE, true);
    Timer timerScreen = Timer(TIME_SCREEN_ON);
    Timer timerBoxOpened = Timer(INTERVAL_ALARM_BOX_OPENED_APRS, true);
    Timer timerSecond = Timer(1000, true);

    CubeCell_NeoPixel *pixels;
#ifdef USE_SCREEN
    SH1107Wire *display;
#endif
    TimerEvent_t *wakeUpEvent;

    void turnScreenOn();
    void turnScreenOff();

    void printJsonSystem(const char *state);
};

#endif //CUBECELL_MONITORING_SYSTEM_H
