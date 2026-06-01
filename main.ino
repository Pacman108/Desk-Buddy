#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include "weather.h"
#include "stocks.h"

// -------------------- TFT ILI9341 pins --------------------

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8

#define TFT_MOSI 11
#define TFT_CLK  12
#define TFT_MISO 13

// -------------------- Button --------------------

#define BUTTON_PIN 4

// -------------------- WiFi --------------------

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// -------------------- TFT object --------------------

// Hardware SPI constructor.
// We call SPI.begin(...) manually in setup() to use your custom pins.
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// -------------------- Colors --------------------

#define BLACK       0x0000
#define WHITE       0xFFFF
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define YELLOW      0xFFE0
#define CYAN        0x07FF
#define DARKCYAN    0x03EF
#define DARKGREY    0x7BEF
#define ORANGE      0xFD20

// -------------------- Screen modes --------------------

enum ScreenMode {
  SCREEN_WEATHER,
  SCREEN_STOCKS
};

ScreenMode currentScreen = SCREEN_WEATHER;

// -------------------- Weather data --------------------

const int CITY_COUNT = 4;

CityConfig cities[CITY_COUNT] = {
  {"Munich", 48.1374, 11.5755},
  {"Berlin", 52.5200, 13.4050},
  {"London", 51.5072, -0.1276},
  {"Delhi", 28.6139, 77.2090}
};

WeatherData weatherData[CITY_COUNT];

// -------------------- Stock data --------------------

const int STOCK_COUNT = 4;

StockConfig stocks[STOCK_COUNT] = {
  {"aapl.us", "AAPL"},
  {"msft.us", "MSFT"},
  {"nvda.us", "NVDA"},
  {"tsla.us", "TSLA"}
};

StockData stockData[STOCK_COUNT];

// -------------------- Button debounce --------------------

unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;

// -------------------- Refresh timers --------------------

unsigned long lastWeatherRefresh = 0;
const unsigned long weatherRefreshInterval = 60000; // 60 seconds

unsigned long lastStockRefresh = 0;
const unsigned long stockRefreshInterval = 60000; // 60 seconds

// -------------------- Display helpers --------------------

void drawHeader(const char* modeTitle) {
  tft.fillRect(0, 0, 320, 45, DARKCYAN);

  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(10, 10);
  tft.print("Desk Buddy");

  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(220, 15);
  tft.print(modeTitle);

  tft.drawLine(0, 45, 320, 45, WHITE);
}

void showMessage(const char* title, const char* line1, const char* line2 = "") {
  tft.fillScreen(BLACK);
  tft.drawRect(0, 0, 320, 240, GREEN);

  tft.setTextColor(CYAN);
  tft.setTextSize(3);
  tft.setCursor(15, 25);
  tft.print(title);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 95);
  tft.print(line1);

  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 130);
  tft.print(line2);
}

// -------------------- Weather display --------------------

void drawWeatherRow(int rowIndex, const WeatherData& data) {
  int y = 70 + rowIndex * 35;

  tft.setTextSize(2);

  // City
  tft.setTextColor(WHITE);
  tft.setCursor(20, y);
  tft.print(data.city);

  // Temperature
  tft.setCursor(140, y);

  if (data.valid) {
    tft.setTextColor(YELLOW);
    tft.print(data.temperature, 1);
    tft.print(" C");
  } else {
    tft.setTextColor(RED);
    tft.print("ERR");
  }

  // Wind
  tft.setTextColor(CYAN);
  tft.setCursor(230, y);

  if (data.valid) {
    tft.print(data.windSpeed, 0);
    tft.print("km/h");
  } else {
    tft.print("--");
  }
}

void drawWeatherScreen() {
  tft.fillScreen(BLACK);

  drawHeader("WEATHER");

  tft.drawRoundRect(10, 58, 300, 150, 8, CYAN);

  for (int i = 0; i < CITY_COUNT; i++) {
    drawWeatherRow(i, weatherData[i]);
  }

  tft.setTextColor(DARKGREY);
  tft.setTextSize(2);
  tft.setCursor(15, 220);
  tft.print("Button: show stocks");
}

// -------------------- Stocks display --------------------

void drawStockRow(int rowIndex, const StockData& data) {
  int y = 70 + rowIndex * 35;

  tft.setTextSize(2);

  // Symbol
  tft.setTextColor(WHITE);
  tft.setCursor(20, y);
  tft.print(data.displayName);

  // Price
  tft.setCursor(115, y);

  if (data.valid) {
    tft.setTextColor(YELLOW);
    tft.print("$");
    tft.print(data.price, 2);
  } else {
    tft.setTextColor(RED);
    tft.print("ERR");
  }

  // Time
  tft.setTextColor(CYAN);
  tft.setCursor(230, y);

  if (data.valid) {
    tft.print(data.time);
  } else {
    tft.print("--");
  }
}

void drawStocksScreen() {
  tft.fillScreen(BLACK);

  drawHeader("STOCKS");

  tft.drawRoundRect(10, 58, 300, 150, 8, ORANGE);

  for (int i = 0; i < STOCK_COUNT; i++) {
    drawStockRow(i, stockData[i]);
  }

  tft.setTextColor(DARKGREY);
  tft.setTextSize(2);
  tft.setCursor(15, 220);
  tft.print("Button: show weather");
}

// -------------------- WiFi --------------------

void connectToWiFi() {
  showMessage("Desk Buddy", "Connecting WiFi...", "");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(20 + (attempts % 12) * 12, 165);
    tft.print(".");

    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    showMessage("WiFi OK", "Connected", WiFi.localIP().toString().c_str());
    delay(700);
  } else {
    Serial.println("WiFi failed");
    showMessage("WiFi Failed", "Check Wokwi-GUEST", "");
  }
}

// -------------------- Weather logic --------------------

void refreshWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    showMessage("WiFi Error", "Not connected", "Retrying WiFi...");
    connectToWiFi();

    if (WiFi.status() != WL_CONNECTED) {
      return;
    }
  }

  showMessage("Weather", "Fetching data...", "Please wait");

  bool ok = fetchAllWeather(cities, weatherData, CITY_COUNT);

  lastWeatherRefresh = millis();

  if (!ok) {
    Serial.println("Some weather requests failed");
  }

  if (currentScreen == SCREEN_WEATHER) {
    drawWeatherScreen();
  }
}

// -------------------- Stock logic --------------------

void refreshStocks() {
  if (WiFi.status() != WL_CONNECTED) {
    showMessage("WiFi Error", "Not connected", "Retrying WiFi...");
    connectToWiFi();

    if (WiFi.status() != WL_CONNECTED) {
      return;
    }
  }

  showMessage("Stocks", "Fetching data...", "Please wait");

  bool ok = fetchAllStocks(stocks, stockData, STOCK_COUNT);

  lastStockRefresh = millis();

  if (!ok) {
    Serial.println("Some stock requests failed");
  }

  if (currentScreen == SCREEN_STOCKS) {
    drawStocksScreen();
  }
}

// -------------------- Screen switching --------------------

void switchScreen() {
  if (currentScreen == SCREEN_WEATHER) {
    currentScreen = SCREEN_STOCKS;

    Serial.println("Switching to STOCKS screen");

    // If stock data has not been fetched yet, fetch it.
    if (lastStockRefresh == 0) {
      refreshStocks();
    } else {
      drawStocksScreen();
    }
  } else {
    currentScreen = SCREEN_WEATHER;

    Serial.println("Switching to WEATHER screen");

    // If weather data has not been fetched yet, fetch it.
    if (lastWeatherRefresh == 0) {
      refreshWeather();
    } else {
      drawWeatherScreen();
    }
  }
}

// -------------------- Button handling --------------------

void handleButton() {
  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();

    Serial.println("Button pressed: switching screen");
    switchScreen();
  }
}

// -------------------- Arduino setup/loop --------------------

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);

  tft.begin();
  tft.setRotation(1);

  showMessage("Desk Buddy", "Booting...", "");
  delay(500);

  connectToWiFi();

  // Initial load: weather first
  currentScreen = SCREEN_WEATHER;
  refreshWeather();
}

void loop() {
  handleButton();

  if (currentScreen == SCREEN_WEATHER) {
    if (millis() - lastWeatherRefresh > weatherRefreshInterval) {
      Serial.println("Auto-refreshing weather");
      refreshWeather();
    }
  }

  if (currentScreen == SCREEN_STOCKS) {
    if (millis() - lastStockRefresh > stockRefreshInterval) {
      Serial.println("Auto-refreshing stocks");
      refreshStocks();
    }
  }
}
