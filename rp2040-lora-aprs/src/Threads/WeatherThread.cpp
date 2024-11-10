#include "Threads/WeatherThread.h"
#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"

WeatherThread::WeatherThread(System *system) : MyThread(system, INTERVAL_WEATHER, PSTR("WEATHER")) {
}

bool WeatherThread::init() {
    if (sensor.begin()) {
        for (uint8_t i = 0; i < 3; i++) {
            delayWdt(150);
            sensor.read_temperature_c(); // To have correct value for first runOnce
        }
        return true;
    }

    return false;
}

bool WeatherThread::runOnce() {
    temperature = sensor.read_temperature_c();
    humidity = sensor.read_humidity();
    pressure = sensor.read_pressure();

    if (pressure <= 700 || pressure >= 1200 || temperature >= 50 || temperature <= -15) {
        Log.warningln(F("[WEATHER] Error ! Temperature: %FC Humidity: %F%% Pressure: %FhPa"), temperature, humidity, pressure);
        begin();
        return false;
    }

    Log.infoln(F("[WEATHER] Temperature: %FC Humidity: %F%% Pressure: %FhPa"), temperature, humidity, pressure);

    return true;
}