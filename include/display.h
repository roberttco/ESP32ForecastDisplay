#ifndef display_h
#define display_h

#include <Arduino.h>
#include <OWMOneCall.h>

void InitDisplay(bool warmBoot);
void PutDisplayToSleep();
void setupZones(uint16_t display_width, uint16_t display_height);
void UpdateEntireDisplay(currentWeather *current, hourlyWeather *hourly);
//void RefreshTimeAndDate();
//void RefreshCurrentConditions(currentWeather *current);
void RefreshCurrentInformation(const currentWeather *current, const struct tm *timeinfo);

void UpdateCurrentTemperature(float temperature, time_t lastChanged);
void UpdateStatusBar(time_t sampleTime, float battery_level);

void DrawCurrentTime(const struct tm *timeinfo);


// Location constants
// These depend on the fonts used and rotation.  These default values may be reset in setup()

const byte ALIGN_X_CENTER   = 0b00000001;
const byte ALIGN_X_LEFT     = 0b00000010;
const byte ALIGN_X_RIGHT    = 0b00000100;
const byte X_LEFT_PAD       = 0b00001000;
const byte ALIGN_Y_MIDDLE   = 0b00010000;
const byte ALIGN_Y_TOP      = 0b00100000;
const byte ALIGN_Y_BOTTOM   = 0b01000000;
const byte Y_BOTTOM_PAD     = 0b10000000;

const uint16_t Y_PAD = 10;
const uint16_t X_PAD = 8;

#endif