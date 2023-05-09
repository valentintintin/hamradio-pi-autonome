#include "WeatherSensors.h"
#include "ArduinoLog.h"
#include "System.h"

WeatherSensors::WeatherSensors(System *system) : system(system) {
}

bool WeatherSensors::begin() {
    dht.begin();

    return true;
}

bool WeatherSensors::update() {
    if (!timer.hasExpired()) {
        return false;
    }

    Log.traceln(F("Fetch weather sensors data"));

    if (!readDht()) {
        Log.warningln(F("Fetch DHT sensor error"));
        system->displayText(PSTR("Weather error"), PSTR("Failed to fetch DHT sensor"));
        return false;
    }

    sprintf_P(bufferText, PSTR("Temperature:%.2fC Humidity:%d%%"), temperature, humidity);
    Log.infoln(PSTR("[WEATHER] %s"), bufferText);
    Log2.infoln(PSTR("[WEATHER] %s"), bufferText);

    system->displayText(PSTR("Weather"), bufferText);

    timer.restart();

    return true;
}

bool WeatherSensors::readDht() {
    dht.temperature().getEvent(&event);

    if (isnan(event.temperature)) {
        Log.warningln(F("Error reading temperature"));
        return false;
    } else {
        temperature = event.temperature;
    }

    dht.humidity().getEvent(&event);

    if (isnan(event.relative_humidity)) {
        Log.warningln(F("Error reading humidity"));
        return false;
    } else {
        humidity = (u_int8_t) event.relative_humidity;
    }

    return true;
}