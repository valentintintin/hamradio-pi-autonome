#include <hardware/rtc.h>
#include "ArduinoLog.h"
#include "System.h"

#ifdef USE_MPPTCHG
    #include "Threads/Energy/EnergyMpptChg.h"
#elifdef USE_INA3221
#include "Threads/Energy/EnergyIna3221.h"
#elifdef USE_BATTERY_ADC
#include "Threads/Energy/EnergyAdc.h"
#else
#include "Threads/Energy/EnergyDummy.h"
#endif

#include "Threads/BlinkerThread.h"
#include "Threads/LdrBoxOpenedThread.h"

#ifdef USE_I2C_SLAVE
#include "I2CSlave.h"
#include "utils.h"
#include "PicoSleep.h"

#endif

System::System() : gpioLed(GpioPin(LED_BUILTIN)), weatherThread(this), command(this), communication(this),
                   watchdogSlaveLoraTx(this), sendPositionThread(this), sendStatusThread(this),
                   sendTelemetriesThread(this) {
    threadController.add(new BlinkerThread(this, &gpioLed));

#ifdef USE_MPPTCHG
    energyThread = new EnergyMpptChg(this);
#elifdef USE_INA3221
    energyThread = new EnergyIna3221(this, (ina3221_ch_t) INA3221_CHANNEL_BATTERY, (ina3221_ch_t) INA3221_CHANNEL_SOLAR);
#elifdef USE_BATTERY_ADC
    energyThread = new EnergyAdc(this, BATTERY_PIN);
#else
    energyThread = new EnergyDummy(this);
#endif
    threadController.add(energyThread);

#ifdef USE_WEATHER
    threadController.add(&weatherThread);
#endif

#ifdef USE_LDR_BOX_OPENED
    threadController.add(new LdrBoxOpenedThread(this, new GpioPin(LDR_BOX_OPENED_PIN, INPUT)));
#endif

    threadController.add(&watchdogSlaveLoraTx);
    threadController.add(&sendPositionThread);
    threadController.add(&sendStatusThread);
    threadController.add(&sendTelemetriesThread);

#ifdef USE_MPPTCHG_WATCHDOG
    watchdogSlaveMpptChg = new WatchdogSlaveMpptChg(this);
    threadController.add(watchdogSlaveMpptChg);
#endif

#ifdef USE_WATCHDOG_MESHTASTIC
    watchdogMeshtastic = new WatchdogMasterPin(this, &gpioMeshtastic, WATCHDOG_MESHTASTIC_TIMEOUT);
    threadController.add(watchdogMeshtastic);
#endif

#ifdef USE_WATCHDOG_LINUX_BOARD
    watchdogLinuxBoard = new WatchdogMasterPin(this, &gpioLinuxBoard, WATCHDOG_LINUX_BOARD_TIMEOUT);
    threadController.add(watchdogLinuxBoard);
#endif
}

bool System::begin() {
    Log.infoln(F("[SYSTEM] Starting"));

    ledBlink();

    gpioLed.setState(HIGH);

    if (watchdog_caused_reboot()) {
        Log.warningln(F("[SYSTEM] Watchdog caused reboot"));
    }

    rtc_init();
    Wire.begin();

#ifdef USE_WATCHDOG
    rp2040.wdt_begin(8300);
    Log.infoln(F("[SYSTEM] Internal watchdog enabled"));
#endif

#ifdef USE_I2C_SLAVE
    I2CSlave::begin(this);
#endif

#ifdef USE_MESHTASTIC
    gpioMeshtastic.setState(true);
#endif

#ifdef USE_RTC
    auto epoch = RTClib::now().unixtime();
    datetime_t datetime;
    epoch_to_datetime(epoch, &datetime);
    rtc_set_datetime(&datetime);
    Log.infoln(F("[SYSTEM] Set internal RTC to date %d/%d/%d %d:%d:%d"), datetime.day, datetime.month, datetime.year, datetime.hour, datetime.min, datetime.sec);
#endif

    communication.begin();

    for(int i = 0; i < MAX_THREADS ; i++) {
        auto thread = (MyThread*) threadController.get(i);
        if (thread != nullptr) {
            if (!thread->begin()) {
                Log.errorln(F("[SYSTEM] Thread init KO"));
                ledBlink(10);
                continue;
            }

            thread->run();
        }
    }

    gpioLed.setState(LOW);

    Log.infoln(F("[SYSTEM] Started"));

    return true;
}

void System::loop() {
    Stream *streamReceived = nullptr;

    if (Serial.available()) {
        streamReceived = &Serial;
        Log.traceln(F("Serial USB incoming"));
    }

    if (streamReceived != nullptr) {
        size_t lineLength = streamReceived->readBytesUntil('\n', bufferText, 150);
        bufferText[lineLength] = '\0';

        Log.traceln(F("[SERIAL] Received %s"), bufferText);

        command.processCommand(bufferText);

        streamReceived->flush();
    } else {
        threadController.run();
    }

    delay(10);

    rp2040.wdt_reset();
}