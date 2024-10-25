#ifndef RP2040_LORA_APRS_COMMUNICATION_H
#define RP2040_LORA_APRS_COMMUNICATION_H

#include <RadioLib.h>
#include "Aprs.h"
#include "config.h"
#include "Timer.h"

class System;

class Communication {
public:
    explicit Communication(System *system);

    bool begin();
    void update();
    void received(uint8_t * payload, uint16_t size, float rssi, float snr);

    bool sendMessage(const char* destination, const char* message, const char* ackToConfirm = nullptr);
    bool sendPosition(const char* comment);
    bool sendStatus(const char* comment);
    bool sendTelemetry();
    bool sendTelemetryParams();

    bool shouldSendTelemetryParams = false;

    inline bool hasError() const {
        return _hasError;
    }
private:
    static volatile bool hasInterrupt;

    static void setHasInterrupt();

    System* system;

    uint8_t buffer[TRX_BUFFER]{};
    AprsPacket aprsPacketTx;
    AprsPacketLite aprsPacketRx;
    uint16_t telemetrySequenceNumber = 0;
    SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RESET, LORA_BUSY, SPI1, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    bool _hasError = false;

    bool send();
    void sent();
};

#endif //RP2040_LORA_APRS_COMMUNICATION_H
