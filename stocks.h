#pragma once

#include <Arduino.h>

struct StockConfig {
  const char* symbol;       // Example: "aapl.us"
  const char* displayName;  // Example: "AAPL"
};

struct StockData {
  const char* symbol;
  const char* displayName;

  float price;
  float open;
  float high;
  float low;
  float volume;

  String date;
  String time;

  bool valid;
};

bool fetchStockQuote(const StockConfig& stockConfig, StockData& stockData);
bool fetchAllStocks(const StockConfig stocks[], StockData stockData[], int stockCount);
