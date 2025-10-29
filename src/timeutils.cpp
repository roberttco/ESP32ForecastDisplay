// RTC demo for ESP32, that includes TZ and DST adjustments
// Get the POSIX style TZ format string from  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// Created by Hardy Maxa
// Complete project details at: https://RandomNerdTutorials.com/esp32-ntp-timezones-daylight-saving/

#include "config.h"

#include <Arduino.h>
#include <time.h>
#include <Timezone.h>

// Time related stuff
const char *NTPSERVER1 = "ntp.home";
const char *NTPSERVER2 = "us.pool.ntp.org";
const char *NTPSERVER3 = "time.nist.gov";

TimeChangeRule myDST = {"MDT", Second, Sun, Mar, 2, -360};    // Daylight time = UTC - 6 hours
TimeChangeRule mySTD = {"MST", First, Sun, Nov, 2, -420};     // Standard time = UTC - 7 hours

Timezone myTZ(myDST, mySTD);

void setTimezone(const char* timezone)
{
    // Serial.printf("  Setting Timezone to %s\n", timezone);
    setenv("TZ", timezone, 1); //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();
}

void initTime(const char* timezone)
{
    struct tm timeinfo;

    // Serial.println("Setting up time");
    configTime(0,0, NTPSERVER1,NTPSERVER2,NTPSERVER3); // First connect to NTP server, with 0 TZ offset
    int retries = 10;
    
    while (retries > 0)
    {
        if (!getLocalTime(&timeinfo))
        {
            //Serial.println("  Failed to obtain time using NTP");
            delay(1000);
            retries--;
        }
        else
        {
            break;
        }
    }
    
    // if (retries > 0)
    //     Serial.println("  Got local time.");
    // else
    //     Serial.println("  Stopped retrying after 10 seconds.");

    // Now we can set the real timezone
    setTimezone(timezone);
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        //Serial.println("Failed to obtain time 1");
        return;
    }
    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst)
{
    struct tm tm;

    tm.tm_year = yr - 1900; // Set date
    tm.tm_mon = month - 1;
    tm.tm_mday = mday;
    tm.tm_hour = hr; // Set time
    tm.tm_min = minute;
    tm.tm_sec = sec;
    tm.tm_isdst = isDst; // 1 or 0
    time_t t = mktime(&tm);
    // Serial.printf("Setting time: %s", asctime(&tm));
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);
}

bool TimestampToTzTimeInfo(time_t in_timestamp, struct tm *out_time)
{
    TimeChangeRule *tcr;        // pointer to the time change rule, use to get TZ abbrev

    time_t tz_timestamp = myTZ.toLocal(in_timestamp, &tcr);

    localtime_r(&tz_timestamp, out_time);

    if(out_time->tm_year > (2016 - 1900)){
        return true;
    }
    return false;
}

bool TimeInfoToTzTimeInfo(struct tm *in_time, struct tm *out_time)
{
    return TimestampToTzTimeInfo(mktime(in_time),out_time);
}

bool getLocalTzTime(struct tm * time_info)
{
    struct tm lt;
    bool rval = getLocalTime(&lt);
    if (rval)
    {
        struct tm ct;
        rval = TimeInfoToTzTimeInfo(&lt,time_info);
    }

    return rval;
}
