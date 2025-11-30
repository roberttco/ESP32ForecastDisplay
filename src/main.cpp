#include <WiFi.h>
#include <OWMOneCall.h>

#include "config.h"
#include "display.h"
#include "timeutils.h"
#include "secret.h"
#include "homeassistant.h"

OWMOneCall weather;

RTC_DATA_ATTR time_t lastBootTime = 0;
RTC_DATA_ATTR int lastForecastStartHour = 99;
bool updateCurrentConditions = false;
bool updateForecast = false;

const char *TIMEZONE = MY_TIMEZONE;
time_t thisBootTime = 0;
bool d9_low_at_boot = false;
bool need_time_init = false;
esp_reset_reason_t reset_reason;

#ifdef APPDEBUG
void verbose_print_reset_reason(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_UNKNOWN:
        Serial.println("Reset reason can not be determined");
        break;
    case ESP_RST_POWERON:
        Serial.println("Reset due to power-on event");
        break;
    case ESP_RST_EXT:
        Serial.println("Reset by external pin (not applicable for ESP32)");
        break;
    case ESP_RST_SW:
        Serial.println("Software reset via esp_restart");
        break;
    case ESP_RST_PANIC:
        Serial.println("Software reset due to exception/panic");
        break;
    case ESP_RST_INT_WDT:
        Serial.println("Reset (software or hardware) due to interrupt watchdog");
        break;
    case ESP_RST_TASK_WDT:
        Serial.println("Reset due to task watchdog");
        break;
    case ESP_RST_WDT:
        Serial.println("Reset due to other watchdogs");
        break;
    case ESP_RST_DEEPSLEEP:
        Serial.println("Reset after exiting deep sleep mode");
        break;
    case ESP_RST_BROWNOUT:
        Serial.println("Brownout reset (software or hardware)");
        break;
    case ESP_RST_SDIO:
        Serial.println("Reset over SDIO");
        break;
    default:
        Serial.print("Reset reason not enumerated: ");
        Serial.println(reason);
        break;
    }
}
#endif

void setup()
{
    struct tm current_time_tm;
    pinMode(D9, INPUT);

#ifdef APPDEBUG
    Serial.begin(115200);
#endif

    reset_reason = esp_reset_reason();
    d9_low_at_boot = (digitalRead(D9) == LOW);

    APPDEBUG_PRINTLN("Initializing time.");
    initTime(TIMEZONE);

    // track the time the system booted
    time(&thisBootTime);

    // now fogire out if we need a full update and/or current conditions update

    if (reset_reason != ESP_RST_DEEPSLEEP || d9_low_at_boot == true)
    {
        updateCurrentConditions = true;
        updateForecast = true;
    }

    if (getLocalTime(&current_time_tm))
    {
        if (current_time_tm.tm_min % TEMPERATURE_INTERVAL_MINUTES == 0)
        {
            updateCurrentConditions |= true;
        }

        APPDEBUG_PRINT("lastForecastStartHour = ");
        APPDEBUG_PRINTLN(lastForecastStartHour);
        APPDEBUG_PRINT("current_time_tm.tm_hour = ");
        APPDEBUG_PRINTLN(current_time_tm.tm_hour);

        // always do a 12-hour forecast update at the top of the hour
        if ((current_time_tm.tm_min == 0) || (lastForecastStartHour != current_time_tm.tm_hour))
        {
            updateForecast |= true;
        }
    }
    else 
    {
        need_time_init = true;
    }

    // set this variable to facilitate interleaving the display init
    // with the WiFi connection bringup and also initialize in the
    // correct manner if this is a warm boot or not.
    bool partialDisplayInit = (reset_reason == ESP_RST_DEEPSLEEP);

#ifdef APPDEBUG
    Serial.println("Delaying 5 seconds.");
    delay(5000);
#endif

// if updating the forecast or conditions, WiFi is required...
    if (updateCurrentConditions || updateForecast || need_time_init)
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

        if (need_time_init)
        {
            APPDEBUG_PRINTLN("Initializing time (again).");
            initTime(TIMEZONE);
        }
    }
    else
    {
        InitDisplay(partialDisplayInit);
    }

#ifdef APPDEBUG
    verbose_print_reset_reason(reset_reason);
    Serial.printf("Wake/boot status:\n     rtc time = %i:%02i:%02i\n     lastBootTime = %u\n     lastForecastStartHour = %u\n     updateCurrentConditions = %s\n     updateforecast = %s\n",
                  current_time_tm.tm_hour, current_time_tm.tm_min, current_time_tm.tm_sec,
                  lastBootTime,
                  lastForecastStartHour,
                  updateCurrentConditions ? "TRUE" : "FALSE",
                  updateForecast ? "TRUE" : "FALSE");
#endif

}

void loop()
{
    if (updateForecast)
    {
        weather.begin(OPEN_WEATHER_MAP_APP_ID, 0, 0, 12, 0, MY_UNITS);
        weather.setLocation(MY_LATITUDE, MY_LONGITUDE);
        weather.getWeather();

        currentWeather current_weather;
        //current_weather.ico = Icon::i01d;
        String lu;
        current_weather.temp = GetHAEntityFloat(TEMPERATURE_ENTITY,&lu);
        //2025-10-11T23:16:41.808443+00:00
        current_weather.description = lu.substring(11,19);

        UpdateEntireDisplay(&current_weather, weather.hrWx);

        // // prepare to verify that the forecast was indeed updated on the hour.  Sometimes it is not for whatever reason.
        // struct tm tm_temp;
        // if (TimestampToTzTimeInfo(weather.hrWx[0].time, &tm_temp))
        // {
        //     lastForecastStartHour = tm_temp.tm_hour;
        // }

        lastForecastStartHour = GetTimeStampHour(weather.hrWx[0].time);
    }
    else
    {
        currentWeather current_weather;
        struct tm current_time;
        bool fetched_time = false;

        if (updateCurrentConditions)
        {
            String lu;
            current_weather.temp = GetHAEntityFloat(TEMPERATURE_ENTITY,&lu);
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

    if (getLocalTime(&timeinfo))
    {
        // wake up 5 seconds after the next minute.ime) / 1000)
        sleep_seconds = 5 + max(10, (int)(60 - timeinfo.tm_sec));
    }

    // update the hourly forecast at the top of each hour but wait an additional 15 seconds for
    // OpenWeathermap to update their API backend.

    if (updateForecast)
    {
        sleep_seconds += 15;
    }

    lastBootTime = thisBootTime;

#ifdef APPDEBUG
    Serial.printf("Pre-sleep status:\n     lastBootTime = %u\n     lastForecastStartHour = %u\n     updateCurrentConditions = %s\n     updateforecast = %s\n",
                  lastBootTime,
                  lastForecastStartHour,
                  updateCurrentConditions ? "TRUE" : "FALSE",
                  updateForecast ? "TRUE" : "FALSE");
#endif    

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
