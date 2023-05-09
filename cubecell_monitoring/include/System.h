#ifndef CUBECELL_MONITORING_SYSTEM_H
#define CUBECELL_MONITORING_SYSTEM_H

#include <cstdint>
#include <CubeCell_NeoPixel.h>
#include <HT_SH1107Wire.h>
#include <DS3231.h>

#include "Communication.h"
#include "Timer.h"
#include "Config.h"
#include "Command.h"
#include "MpptMonitor.h"
#include "WeatherSensors.h"
#include "Gpio.h"

class System {
public:
    explicit System(SH1107Wire *display, CubeCell_NeoPixel *pixels);

    bool begin(RadioEvents_t *radioEvents);
    void update();
    void userButton();
    void wakeUp();
    void sleep();

    void turnOnRGB(uint32_t color);
    void turnOffRGB();

    void turnScreenOn();
    void turnScreenOff();
    void displayText(const char* title, const char* content, uint16_t pause = TIME_SCREEN) const;

    Communication *communication;
    SH1107Wire *display;
    Command command;
    MpptMonitor mpptMonitor;
    WeatherSensors weatherSensors;
    Gpio gpio;

    bool forceSendTelemetry = false;
private:
    char bufferText[256]{};
    bool screenOn = false;

    Timer timerPosition = Timer(INTERVAL_POSITION, true);
    Timer timerTelemetry = Timer(INTERVAL_REFRESH_APRS, false);
    Timer timerTime = Timer(INTERVAL_TIME, true);

    CubeCell_NeoPixel *pixels;
    DS3231 RTC;

    void timeUpdate();

    static void nowToString(char *result);
};

#endif //CUBECELL_MONITORING_SYSTEM_H