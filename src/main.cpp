/*
   Open Weather One Call Library
   v4.0.0
   Copyright 2020=2024 - Jessica Hershey
   www.github.com/JHershey69

   One Call API key at www.openweathermap.org
   Google Developer Key no longer required.

   Simple_Latitude_Longitude_Weather_Example.ino

   Returns ALL information and uses Latitude/Longitude, CITY ID, or IP Address
   If using a hotspot to connect your ESP32 to the WWW your results for IP
   search maybe be radically out of range. Please consult the documentation
   for use and variables available in the returned weather message

*/

// ===== Required libraries (Other required libraries are installed in header file)
#include <WiFi.h>
#include <OWMOneCall.h>

#include "app.h"
#include "display.h"
#include "timeutils.h"
#include "secret.h"
#include "homeassistant.h"

// ======= END REQUIRED LIBRARIES =======================================

OWMOneCall weather;
OWMUnits units = IMPERIAL;

RTC_DATA_ATTR time_t lastBootTime = 0;
RTC_DATA_ATTR bool warm_boot = true;
RTC_DATA_ATTR bool updateCurrentConditions = false;
RTC_DATA_ATTR bool updateForecast = false;

const char *TIMEZONE = "America/Denver";
time_t thisBootTime = 0;
bool d9_low_at_boot = false;
esp_reset_reason_t reset_reason;

void verbose_print_reset_reason(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        APPDEBUG_PRINTLN("Reset reason can not be determined");
        break;
    case ESP_RST_POWERON:
        APPDEBUG_PRINTLN("Reset due to power-on event");
        break;
    case ESP_RST_EXT:
        APPDEBUG_PRINTLN("Reset by external pin (not applicable for ESP32)");
        break;
    case ESP_RST_SW:
        APPDEBUG_PRINTLN("Software reset via esp_restart");
        break;
    case ESP_RST_PANIC:
        APPDEBUG_PRINTLN("Software reset due to exception/panic");
        break;
    case ESP_RST_INT_WDT:
        APPDEBUG_PRINTLN("Reset (software or hardware) due to interrupt watchdog");
        break;
    case ESP_RST_TASK_WDT:
        APPDEBUG_PRINTLN("Reset due to task watchdog");
        break;
    case ESP_RST_WDT:
        APPDEBUG_PRINTLN("Reset due to other watchdogs");
        break;
    case ESP_RST_DEEPSLEEP:
        APPDEBUG_PRINTLN("Reset after exiting deep sleep mode");
        break;
    case ESP_RST_BROWNOUT:
        APPDEBUG_PRINTLN("Brownout reset (software or hardware)");
        break;
    case ESP_RST_SDIO:
        APPDEBUG_PRINTLN("Reset over SDIO");
        break;
    default:
        APPDEBUG_PRINT("Reset reason not enumerated: ");
        APPDEBUG_PRINTLN(reason);
        break;
    }
}

void setup()
{
    pinMode(D9, INPUT);

#ifdef APPDEBUG
    delay(5000);
#endif

    reset_reason = esp_reset_reason();
    d9_low_at_boot = (digitalRead(D9) == LOW);

    if (reset_reason != ESP_RST_DEEPSLEEP || d9_low_at_boot == true)
    {
        updateCurrentConditions = true;
        updateForecast = true;
    }

    // set this variable to facilitate interleaving the display init
    // with the WiFi connection bringup and also initialize in the
    // correct manner if this is a warm boot or not.
    bool partialDisplayInit = (reset_reason == ESP_RST_DEEPSLEEP);

    // if updating the forecast or conditions, WiFi is required...
    if (updateCurrentConditions || updateForecast)
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        InitDisplay(partialDisplayInit);

        int TryNum = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            TryNum++;
            if (TryNum > 20)
            {
                APPDEBUG_PRINT("\nUnable to connect to WiFi after 10 seconds. Please check your parameters.\n");
                WiFi.eraseAP();
                delay(2000);
                ESP.restart();
            }
        }
#ifdef APPDEBUG
        Serial.printf("WiFi connected to: %s\n", WIFI_SSID);
#endif
    }
    else
    {
        InitDisplay(partialDisplayInit);
    }

    APPDEBUG_PRINTLN("Initializing time.");
    initTime(TIMEZONE);

    // track the time the system booted
    time(&thisBootTime);
}

void loop()
{
    if (reset_reason != ESP_RST_DEEPSLEEP || warm_boot == false || updateForecast)
    {
        weather.begin(OPEN_WEATHER_MAP_APP_ID, 0, 0, 12, 0, IMPERIAL);
        weather.setLocation(OPEN_WEATHER_MAP_LOCATTION_LAT, OPEN_WEATHER_MAP_LOCATTION_LON);
        weather.getWeather();

        currentWeather current_weather;
        //current_weather.ico = Icon::i01d;
        String lu;
        current_weather.temp = GetTemperature(&lu);
        //2025-10-11T23:16:41.808443+00:00
        current_weather.description = lu.substring(11,19);

        UpdateEntireDisplay(&current_weather, weather.hrWx);

        updateForecast = false;
        updateCurrentConditions = false;
    }
    else if (warm_boot)
    {
        currentWeather current_weather;
        struct tm current_time;
        bool fetched_time = false;

        if (updateCurrentConditions)
        {
            String lu;
            current_weather.temp = GetTemperature(&lu);
            //2025-10-11T23:16:41.808443+00:00
            current_weather.description = lu.substring(11,19);

            APPDEBUG_PRINT("Retrieved new current temperature: ");
            APPDEBUG_PRINTLN(current_weather.temp);
        }

        fetched_time = getLocalTzTime(&current_time);
        
        RefreshCurrentInformation((updateCurrentConditions ? &current_weather : nullptr),
                                  (fetched_time ? &current_time : nullptr));
    }

    struct tm timeinfo;
    int sleep_seconds = 60;

    if (!getLocalTime(&timeinfo))
    {
        // if the time couldnt be retrieved, then try again next boot.
        warm_boot = false;
    }
    else
    {
        // wake up 5 seconds after the next minute.ime) / 1000)
        sleep_seconds = 5 + max(10, (int)(60 - timeinfo.tm_sec));
    }

    // update current conditions every 5 minutes
    if (timeinfo.tm_min % HA_UPDATE_INTERVAL_MINUTES == 0)
    {
        updateCurrentConditions = true;
    }

    // update the hourly forecast at the top of each hour but wait an additional 15 seconds for
    // OpenWeathermap to update their API backend.
    if (timeinfo.tm_min == 59)
    {
        updateForecast = true;
        sleep_seconds += 15;
    }

    verbose_print_reset_reason(reset_reason);

#ifdef APPDEBUG
    Serial.printf("Pre-sleep status:\n     warm_boot = %s\n     rtc time = %i:%02i:%02i\n     sleep_seconds = %i\n     lastBootTime = %u\n     updateCurrentConditions = %s\n     updateforecast = %s\n",
                  warm_boot ? "TRUE" : "FALSE",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                  sleep_seconds,
                  lastBootTime,
                  updateCurrentConditions ? "TRUE" : "FALSE",
                  updateForecast ? "TRUE" : "FALSE");
#endif

    lastBootTime = thisBootTime;

    if (d9_low_at_boot)
    {
#ifdef APPDEBUG
        Serial.printf("Staying awake this time.  Will resume in %i seconds.\n",sleep_seconds);
        delay(sleep_seconds * 1000);
#endif
    }
    else
    {
        esp_sleep_enable_timer_wakeup((sleep_seconds) * 1e6);
#ifdef APPDEBUG
        Serial.printf("Going to sleep.  Will resume in %i seconds.\n",sleep_seconds);
        Serial.flush();
#endif
        delay(100);
        esp_deep_sleep_start();
    }
}
