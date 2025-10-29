#pragma once

//#define APPDEBUG
//#define DEBUG_HTTPCLIENT

// #########################
// # Debug stuff
// #########################
#ifdef APPDEBUG
#define APPDEBUG_PRINT(x) Serial.print(x)
#define APPDEBUG_PRINTLN(x) Serial.println(x)
#else
#define APPDEBUG_PRINT(x)
#define APPDEBUG_PRINTLN(x)
#endif

#define MY_UNITS IMPERIAL // or METRIC
#define MY_LATITUDE 40.4865477
#define MY_LONGITUDE -105.0804652
#define MY_TIMEZONE "America/Denver"    // seems like this could be derived form the lat/long

#define FORECAST_INTERVAL_MINUTES 60
#define TEMPERATURE_INTERVAL_MINUTES 5

