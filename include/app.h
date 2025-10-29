#ifndef __app_h
#define __app_h

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

#endif