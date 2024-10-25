#ifndef RP2040_LORA_APRS_CONFIG_H
#define RP2040_LORA_APRS_CONFIG_H

#define BUFFER_LENGTH 512

#define LORA_SCK 14  // GPIO14
#define LORA_MISO 24 // GPIO24
#define LORA_MOSI 15 // GPIO15
#define LORA_CS 13   // GPIO13
#define LORA_RESET 23         // GPIO23
#define LORA_BUSY 18          // GPIO18
#define LORA_DIO1 16          // GPIO16
#define LORA_DIO4 17          // GPIO17

#define RF_FREQUENCY 433.775
#define LORA_BANDWIDTH 125
#define LORA_SPREADING_FACTOR 12 // [SF7..SF12]
#define LORA_CODINGRATE 5 // [4/5, 4/6, 4/7, 4/8]
#define TX_OUTPUT_POWER 22
#define TRX_BUFFER 253 // 256 - 3 because 3 bytes for LoRa APRS

extern char bufferText[BUFFER_LENGTH];

#endif


/*
 * Arduino log : format
* %s	display as string (char*)
* %S    display as string from flash memory (__FlashStringHelper* or char[] PROGMEM)
* %c	display as single character
* %C    display as single character or as hexadecimal value (prefixed by `0x`) if not a printable character
* %d	display as integer value
* %l	display as long value
* %u	display as unsigned long value
* %x	display as hexadecimal value
* %X	display as hexadecimal value prefixed by `0x` and leading zeros
* %b	display as binary number
* %B	display as binary number, prefixed by `0b`
* %t	display as boolean value "t" or "f"
* %T	display as boolean value "true" or "false"
* %D,%F display as double value
* %p    display a  printable object
 */