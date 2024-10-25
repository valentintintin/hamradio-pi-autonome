#ifndef RP2040_LORA_APRS_SENDTELEMETRIESTHREAD_H
#define RP2040_LORA_APRS_SENDTELEMETRIESTHREAD_H

#include "Threads/SendThread.h"

class System;

class SendTelemetriesThread : public SendThread {
public:
    explicit SendTelemetriesThread(System *system);
protected:
    bool send() override;
};


#endif //RP2040_LORA_APRS_SENDTELEMETRIESTHREAD_H
