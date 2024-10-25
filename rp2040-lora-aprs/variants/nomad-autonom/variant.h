#ifndef RP2040_LORA_APRS_VARIANT_H
#define RP2040_LORA_APRS_VARIANT_H

#define USE_INA3221
#define INA3221_CHANNEL_BATTERY 0
#define INA3221_CHANNEL_SOLAR 1

#define APRS_CALLSIGN "F4HVV-14"
#define APRS_PATH "WIDE1-1"
#define APRS_DESTINATION "APLV1" // APL is LoRa : http://www.aprs.org/aprs11/tocalls.txt --> V1 is a new for this project
#define APRS_SYMBOL '#'
#define APRS_SYMBOL_TABLE 'L'
#define APRS_LATITUDE 45.5180287
#define APRS_LONGITUDE 5.1216384
#define APRS_ALTITUDE 470
#define APRS_COMMENT "f4hvv.valentin-saugnier.fr"
#define APRS_STATUS "Digi & Meshtastic Gaulix 868"
#define APRS_TELEMETRY_IN_POSITION true

#endif //RP2040_LORA_APRS_VARIANT_H
