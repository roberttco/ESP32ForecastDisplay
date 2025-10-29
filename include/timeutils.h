#pragma once

// RTC demo for ESP32, that includes TZ and DST adjustments
// Get the POSIX style TZ format string from  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// Created by Hardy Maxa
// Complete project details at: https://RandomNerdTutorials.com/esp32-ntp-timezones-daylight-saving/

void setTimezone(const char* timezone);
void initTime(const char* timezone);
void printLocalTime();
void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst);
bool TimestampToTzTimeInfo(time_t in_timestamp, struct tm *out_time);
bool TimeInfoToTzTimeInfo(struct tm *in_time, struct tm *out_time);
bool getLocalTzTime(struct tm * time_info);