#include "weather.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

static String buildWeatherUrl(float latitude, float longitude) {
  String url = "http://api.open-meteo.com/v1/forecast?";
  url += "latitude=";
  url += String(latitude, 4);
  url += "&longitude=";
  url += String(longitude, 4);
  url += "&current=temperature_2m,wind_speed_10m";
  return url;
}

bool fetchWeatherForCity(const CityConfig& cityConfig, WeatherData& weatherData) {
  weatherData.city = cityConfig.name;
  weatherData.temperature = 0.0;
  weatherData.windSpeed = 0.0;
  weatherData.valid = false;

  String url = buildWeatherUrl(cityConfig.latitude, cityConfig.longitude);

  Serial.print("Fetching weather for ");
  Serial.print(cityConfig.name);
  Serial.print(": ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode <= 0) {
    Serial.print("HTTP request failed for ");
    Serial.print(cityConfig.name);
    Serial.print(". Error code: ");
    Serial.println(httpCode);

    http.end();
    return false;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Unexpected HTTP status for ");
    Serial.print(cityConfig.name);
    Serial.print(": ");
    Serial.println(httpCode);

    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<1024> doc;

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON parse failed for ");
    Serial.print(cityConfig.name);
    Serial.print(": ");
    Serial.println(error.c_str());

    return false;
  }

  weatherData.temperature = doc["current"]["temperature_2m"] | 0.0;
  weatherData.windSpeed = doc["current"]["wind_speed_10m"] | 0.0;
  weatherData.valid = true;

  Serial.print(cityConfig.name);
  Serial.print(" temp: ");
  Serial.print(weatherData.temperature);
  Serial.print(" C, wind: ");
  Serial.print(weatherData.windSpeed);
  Serial.println(" km/h");

  return true;
}

bool fetchAllWeather(const CityConfig cities[], WeatherData weatherData[], int cityCount) {
  bool allOk = true;

  for (int i = 0; i < cityCount; i++) {
    bool ok = fetchWeatherForCity(cities[i], weatherData[i]);

    if (!ok) {
      allOk = false;
    }

    delay(200);
  }

  return allOk;
}
