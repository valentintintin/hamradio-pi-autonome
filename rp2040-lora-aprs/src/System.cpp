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
#endif

System::System() : gpioLed(GpioPin(LED_BUILTIN)), weatherThread(this), command(this), communication(this),
                   watchdogSlaveLoraTx(this), sendPositionThread(this), sendStatusThread(this),
                   sendTelemetriesThread(this) {
    threadController.add(new BlinkerThread(this, &gpioLed));
    threadController.add(&watchdogSlaveLoraTx);
    threadController.add(&sendPositionThread);
    threadController.add(&sendStatusThread);
    threadController.add(&sendTelemetriesThread);

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

#ifdef USE_MPPTCHG_WATCHDOG
    watchdogSlaveMpptChg = new WatchdogSlaveMpptChg(this);
    threadController.add(watchdogSlaveMpptChg);
#endif

#ifdef USE_WATCHDOG_MESHTASTIC
    watchdogMeshtastic = new WatchdogMasterPin(this, &gpioMeshtastic, WATCHDOG_MESHTASTIC_TIMEOUT);
    threadController.add(watchdogMeshtastic);
#endif

#ifdef USE_LDR_BOX_OPENED
    threadController.add(new LdrBoxOpenedThread(this, new GpioPin(LDR_BOX_OPENED_PIN, INPUT)));
#endif

#ifdef USE_WATCHDOG_LINUX_BOARD
    watchdogLinuxBoard = new WatchdogMasterPin(this, &gpioLinuxBoard, WATCHDOG_LINUX_BOARD_TIMEOUT);
    threadController.add(watchdogLinuxBoard);
#endif
}

bool System::begin() {
    Log.infoln(F("[SYSTEM] Starting"));

    gpioLed.setState(HIGH);

    SPI1.setSCK(LORA_SCK);
    SPI1.setTX(LORA_MOSI);
    SPI1.setRX(LORA_MISO);
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    SPI1.begin(false);

    communication.begin();
    weatherThread.begin();
    energyThread->begin();

#ifdef USE_MPPTCHG_WATCHDOG
    watchdogSlaveMpptChg->begin();
#endif

#ifdef USE_I2C_SLAVE
    I2CSlave::begin(this);
#endif

    Log.infoln(F("[SYSTEM] Started"));

    gpioLed.setState(LOW);

    return true;
}

void System::loop() {
    Stream *streamReceived = nullptr;

    if (Serial.available()) {
        streamReceived = &Serial;
        Log.verboseln(F("Serial USB incoming"));
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
}