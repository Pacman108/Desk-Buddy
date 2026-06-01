#include "stocks.h"

#include <HTTPClient.h>

static String buildStockUrl(const char* symbol) {
  String url = "http://stooq.com/q/l/?s=";
  url += symbol;

  // s  = symbol
  // d2 = date
  // t2 = time
  // o  = open
  // h  = high
  // l  = low
  // c  = close/current price
  // v  = volume
  // n  = name
  // e=csv gives CSV output
  url += "&f=sd2t2ohlcvn&h&e=csv";

  return url;
}

static String getCsvField(const String& line, int fieldIndex) {
  int currentField = 0;
  int startIndex = 0;

  for (int i = 0; i <= line.length(); i++) {
    bool atEnd = (i == line.length());
    bool atComma = (!atEnd && line.charAt(i) == ',');

    if (atComma || atEnd) {
      if (currentField == fieldIndex) {
        return line.substring(startIndex, i);
      }

      currentField++;
      startIndex = i + 1;
    }
  }

  return "";
}

bool fetchStockQuote(const StockConfig& stockConfig, StockData& stockData) {
  stockData.symbol = stockConfig.symbol;
  stockData.displayName = stockConfig.displayName;

  stockData.price = 0.0;
  stockData.open = 0.0;
  stockData.high = 0.0;
  stockData.low = 0.0;
  stockData.volume = 0.0;

  stockData.date = "";
  stockData.time = "";
  stockData.valid = false;

  String url = buildStockUrl(stockConfig.symbol);

  Serial.print("Fetching stock quote for ");
  Serial.print(stockConfig.displayName);
  Serial.print(": ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();

  if (httpCode <= 0) {
    Serial.print("HTTP request failed for ");
    Serial.print(stockConfig.displayName);
    Serial.print(". Error code: ");
    Serial.println(httpCode);

    http.end();
    return false;
  }

  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Unexpected HTTP status for ");
    Serial.print(stockConfig.displayName);
    Serial.print(": ");
    Serial.println(httpCode);

    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.println("Stock CSV response:");
  Serial.println(payload);

  /*
    Expected CSV format:

    Symbol,Date,Time,Open,High,Low,Close,Volume,Name
    AAPL.US,2026-05-29,22:00:11,xxx,xxx,xxx,xxx,xxx,APPLE
  */

  int firstNewline = payload.indexOf('\n');

  if (firstNewline < 0) {
    Serial.println("CSV parse failed: no newline found");
    return false;
  }

  String dataLine = payload.substring(firstNewline + 1);
  dataLine.trim();

  if (dataLine.length() == 0) {
    Serial.println("CSV parse failed: empty data line");
    return false;
  }

  String symbolField = getCsvField(dataLine, 0);
  String dateField = getCsvField(dataLine, 1);
  String timeField = getCsvField(dataLine, 2);
  String openField = getCsvField(dataLine, 3);
  String highField = getCsvField(dataLine, 4);
  String lowField = getCsvField(dataLine, 5);
  String closeField = getCsvField(dataLine, 6);
  String volumeField = getCsvField(dataLine, 7);

  if (closeField.length() == 0 || closeField == "N/D") {
    Serial.print("Invalid close price for ");
    Serial.println(stockConfig.displayName);
    return false;
  }

  stockData.date = dateField;
  stockData.time = timeField;

  stockData.open = openField.toFloat();
  stockData.high = highField.toFloat();
  stockData.low = lowField.toFloat();
  stockData.price = closeField.toFloat();
  stockData.volume = volumeField.toFloat();

  stockData.valid = true;

  Serial.print(stockConfig.displayName);
  Serial.print(" price: ");
  Serial.print(stockData.price);
  Serial.print(" date: ");
  Serial.print(stockData.date);
  Serial.print(" time: ");
  Serial.println(stockData.time);

  return true;
}

bool fetchAllStocks(const StockConfig stocks[], StockData stockData[], int stockCount) {
  bool allOk = true;

  for (int i = 0; i < stockCount; i++) {
    bool ok = fetchStockQuote(stocks[i], stockData[i]);

    if (!ok) {
      allOk = false;
    }

    delay(200);
  }

  return allOk;
}
