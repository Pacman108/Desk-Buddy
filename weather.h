#pragma once

#include <Arduino.h>

struct CityConfig {
  const char* name;
  float latitude;
  float longitude;
};

struct WeatherData {
  const char* city;
  float temperature;
  float windSpeed;
  bool valid;
};

bool fetchWeatherForCity(const CityConfig& cityConfig, WeatherData& weatherData);
bool fetchAllWeather(const CityConfig cities[], WeatherData weatherData[], int cityCount);
