#include "Threads/WeatherThread.h"
#include "ArduinoLog.h"
#include "System.h"
#include "utils.h"

WeatherThread::WeatherThread(System *system) : MyThread(system, INTERVAL_WEATHER, PSTR("WEATHER")) {
}

bool WeatherThread::init() {
    return sensor.begin();
}

bool WeatherThread::runOnce() {
    temperature = sensor.read_temperature_c();
    humidity = sensor.read_humidity();
    pressure = sensor.read_pressure();

    if (pressure <= 700 || pressure >= 1200 || temperature >= 50 || temperature <= -15) {
        Log.warningln(F("[WEATHER] Error ! Temperature: %FC Humidity: %F%% Pressure: %FhPa"), temperature, humidity, pressure);
        return false;
    }

    Log.infoln(F("[WEATHER] Temperature: %FC Humidity: %F%% Pressure: %FhPa"), temperature, humidity, pressure);

    return true;
}