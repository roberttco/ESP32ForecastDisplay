#include "app.h"
#include "display.h"
#include "timeutils.h"

#include <time.h>
#include <GxEPD2_BW.h>

#include "fonts/RadioCanada_Medium18pt7b.h"
const GFXfont *currentDateFont = &RadioCanada_Medium18pt7b;
const GFXfont *forecastTimeFont = &RadioCanada_Medium18pt7b;

#include "fonts/RadioCanada_Medium32pt7b-numbersonly.h"
const GFXfont *forecastTempFont = &RadioCanada_Medium32pt7b;

//#include "fonts/RadioCanada_Regular40pt7b.h"


#include "fonts/Meteocons_Regular_72.h"
const GFXfont *forecastConditionFont = &Meteocons_Regular_72;

// #include "fonts/Meteocons_Regular_96.h"
// const GFXfont *currentConditionsFont = &Meteocons_Regular_96;

// #include "fonts/RadioCanada_Medium72pt7b.h"
// #include "fonts/Viga_Regular72pt7b.h"
// #include "fonts/CantoraOne_Regular72pt7b.h"
//#include "fonts/BakbakOne_Regular72pt7b-numbersonly.h"
#include "fonts/NotoSansLinearB_Regular72pt7b-onlynumbers.h"
const GFXfont *currentTempFont = &NotoSansLinearB_Regular72pt7b;

//#include "fonts/Ubuntu_Medium84pt7b.h"
const GFXfont *currentTimeFont = &NotoSansLinearB_Regular72pt7b;

// 7.5'' EPD Module (Seeed Studio)
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS=*/3, /*DC=*/5, /*RST=*/2, /*BUSY=*/4)); // 800x480, SSD1675A

// zone heights should be a multiple of 8 with rotation = 1 to accommodate the display controller
// behavior.
typedef struct _Zone
{
    uint8_t border;
    uint8_t border_radius;
    int16_t x, y, w, h;
} Zone;

// region zones with no border
//#define REGIONBOXES // draw boxes arround regions

Zone nzones[] = {
    {0, 0, 0,   0,   500, 160}, // current time
    {0, 0, 0,   160, 500, 80},  // current date

    {0, 0, 500, 0,   300, 120}, // current condition icon
    {0, 0, 500, 120, 300, 120}, // current temperature

    {0, 0, 500, 0,   300, 160}, // alternate current layout with
    {0, 0, 500, 160, 300, 80},  // temp on top and time on bottom

    {0, 0, 0,   240, 133, 60},  // hour 1 time
    {0, 0, 133, 240, 133, 60},  // hour 2 time
    {0, 0, 266, 240, 133, 60},  // hour 3 time
    {0, 0, 399, 240, 133, 60},  // hour 4 time
    {0, 0, 532, 240, 133, 60},  // hour 5 time
    {0, 0, 665, 240, 133, 60},  // hour 6 time

    {0, 0, 0,   300, 133, 80},  // hour 1 temperature
    {0, 0, 133, 300, 133, 80},  // hour 2 temperature
    {0, 0, 266, 300, 133, 80},  // hour 3 temperature
    {0, 0, 399, 300, 133, 80},  // hour 4 temperature
    {0, 0, 532, 300, 133, 80},  // hour 5 temperature
    {0, 0, 665, 300, 133, 80},  // hour 6 temperature

    {0, 0, 0,   380, 133, 95}, // hour 1 conditions/icon
    {0, 0, 133, 380, 133, 95}, // hour 2 conditions/icon
    {0, 0, 266, 380, 133, 95}, // hour 3 conditions/icon
    {0, 0, 399, 380, 133, 95}, // hour 4 conditions/icon
    {0, 0, 532, 380, 133, 95}, // hour 5 conditions/icon
    {0, 0, 665, 380, 133, 95}, // hour 6 conditions/icon
};

#define CURRENT_TIME_ZONE 0
#define CURRENT_DATE_ZONE 1
#define CONDITIONS_ZONE_TOP 2
#define CONDITIONS_ZONE_BOTTOM 3
#define ALT_CONDITIONS_TEMP 4
#define ALT_CONDITIONS_TIME 5

const int hourly_forecast_spacing = 2; // two hours per forecast zone
const int hourly_forecast_slots = 6;
const int forcast_time_zone_offset = 6;
const int forcast_temp_zone_offset = 12;
const int forcast_icon_zone_offset = 18;

Zone superregion_zones[] = {
    {0, 0, 0,   0,   500, 240},
    {0, 0, 500, 0,   300, 240},
    {0, 0, 5,   240, 790, 240},
    {0, 0, 0,   0,   800, 240}, // the entire top of the screen

    {2, 5, 3,   240, 130, 235},
    {2, 5, 134, 240, 132, 235},
    {2, 5, 267, 240, 132, 235},
    {2, 5, 400, 240, 132, 235},
    {2, 5, 533, 240, 132, 235},
    {2, 5, 666, 240, 130, 235},
};

#define SUPERREGION_TIMEDATE 0
#define SUPERREGION_CONDITIONS 1
#define SUPERREGION_FORECAST 2
#define SUPERREGION_CURRENT 3

extern int myTimezone;

void InitDisplay(bool warmBoot)
{
    if (warmBoot)
    {
        APPDEBUG_PRINTLN("Init display after warm boot");
        display.init(9600, false, 2, false);
    }
    else
    {
        APPDEBUG_PRINTLN("Init display");
        display.init(9600, true, 2, false);
    }

    display.setRotation(0);
    display.setPartialWindow(0, 0, display.width(), display.height());
}

void PutDisplayToSleep()
{
    display.hibernate();
}

void GetAlignedStringCoordinatesInZone(String str, const GFXfont *font, Zone *zonemap, int zone, byte alignment = ALIGN_Y_TOP | ALIGN_X_LEFT, int16_t *sx = nullptr, int16_t *sy = nullptr)
{
    int16_t zone_x = zonemap[zone].x;
    int16_t zone_y = zonemap[zone].y;
    int16_t zone_w = zonemap[zone].w;
    int16_t zone_h = zonemap[zone].h;

#ifdef APPDEBUG
    Serial.printf("Zone: %i, align=%2i, x=%3i, y=%3i, w=%3i, h=%3i, ", zone, alignment, zone_x, zone_y, zone_w, zone_h);
#endif

    int16_t tbx, tby;
    uint16_t tbw, tbh;

    int16_t x = 0, y = 0;

    const char *c_str = str.c_str();

    display.setFont(font);
    display.getTextBounds(c_str, 0, 0, &tbx, &tby, &tbw, &tbh);

    // do X first
    if ((alignment & ALIGN_X_CENTER) != 0)
    {
        x = zone_x - tbx + (zone_w - tbw) / 2;
    }
    else if ((alignment & ALIGN_X_RIGHT) != 0)
    {
        x = zone_x + zone_w - tbw - tbx;
    }
    else if ((alignment & ALIGN_X_LEFT) != 0)
    {
        x = zone_x;

        if ((alignment & X_LEFT_PAD) != 0)
        {
            x += X_PAD;
        }
    }

    // now y
    if ((alignment & ALIGN_Y_MIDDLE) != 0)
    {
        y = zone_y + (zone_h + tbh) / 2;
    }
    else if ((alignment & ALIGN_Y_BOTTOM) != 0)
    {
        y = zone_y + zone_h;

        if ((alignment & Y_BOTTOM_PAD) != 0)
        {
            y -= Y_PAD;
        }
    }
    else if ((alignment & ALIGN_Y_TOP) != 0)
    {
        y = zone_y + tbh;
    }

    if (sx != nullptr && sy != nullptr)
    {
        *sx = x;
        *sy = y;
    }

#ifdef APPDEBUG
    Serial.printf("tbx=%3i, tby=%3i, tbw=%3i, tbh=%3i,      x=%3i, sx=%3i, y=%3i, sy=%3i, val='%s'\n", tbx, tby, tbw, tbh, x, *sx, y, *sy, c_str);
#endif
}

void DrawBorder(Zone *zonemap, int zone, uint16_t color)
{
    if (zonemap[zone].border > 0)
    {
        for (uint8_t t = 0; t < zonemap[zone].border; t++)
        {
            int16_t width = zonemap[zone].w - 2 * t;
            int16_t height = zonemap[zone].h - 2 * t;
            int16_t x = zonemap[zone].x + t + 1;
            int16_t y = zonemap[zone].y + t + 1;

            display.drawRoundRect(x, y, width, height, zonemap[zone].border_radius, GxEPD_BLACK);
        }
    }
}

void FillZone(Zone *zonemap, int zone, uint16_t color, bool insideBorder = false)
{
    display.fillRect(zonemap[zone].x + (insideBorder ? zonemap[zone].border : 0),
                     zonemap[zone].y + (insideBorder ? zonemap[zone].border : 0),
                     zonemap[zone].w - (insideBorder ? zonemap[zone].border * 2 : 0),
                     zonemap[zone].h - (insideBorder ? zonemap[zone].border * 2 : 0),
                     color);
}

void SetWindow(Zone *zonemap, int zone, bool insideBorder = false)
{
    display.setPartialWindow(zonemap[zone].x + (insideBorder ? zonemap[zone].border : 0),
                             zonemap[zone].y + (insideBorder ? zonemap[zone].border : 0),
                             zonemap[zone].w - (insideBorder ? zonemap[zone].border * 2 : 0),
                             zonemap[zone].h - (insideBorder ? zonemap[zone].border * 2 : 0));
}

char ConvertIconToChar(const char *icon)
{
    APPDEBUG_PRINT ("Icon: ");
    APPDEBUG_PRINTLN(icon);

    if (icon == nullptr)
        return '?'; // default unknown
    String iconStr = String(icon);
    if (iconStr == "01d")
        return 'A';
    else if (iconStr == "01n")
        return 'B';
    else if (iconStr == "02d")
        return 'C';
    else if (iconStr == "02n")
        return 'D';
    else if (iconStr == "03d" || iconStr == "03n")
        return 'E';
    else if (iconStr == "04d" || iconStr == "04n")
        return 'F';
    else if (iconStr == "09d" || iconStr == "09n")
        return 'G';
    else if (iconStr == "10d")
        return 'H';
    else if (iconStr == "10n")
        return 'I';
    else if (iconStr == "11d" || iconStr == "11n")
        return 'J';
    else if (iconStr == "13d" || iconStr == "13n")
        return 'K';
    else if (iconStr == "50d" || iconStr == "50n")
        return 'L';
    return '?'; // default unknown
}

char ConvertIconToChar(Icon icon)
{
    // using this font https://github.com/mediastuttgart/meteocons.git
    APPDEBUG_PRINT ("Icon: ");
    APPDEBUG_PRINTLN(icon);

    switch (icon)
    {
    case Icon::i01d:
        return 'B';
    case Icon::i02d:
        return 'H';
    case Icon::i03d:
        return 'N';
    case Icon::i04d:
        return 'Y';
    case Icon::i09d:
        return 'R';
    case Icon::i10d:
        return 'X';
    case Icon::i11d:
        return 'O';
    case Icon::i13d:
        return 'W';
    case Icon::i50d:
        return 'M';

    case Icon::i01n:
        return '2'; 
    case Icon::i02n:
        return '4';
    case Icon::i03n:
        return '5';
    case Icon::i04n:
        return '%';
    case Icon::i09n:
        return '8';
    case Icon::i10n:
        return '$';
    case Icon::i11n:
        return '6';
    case Icon::i13n:
        return '#';
    case Icon::i50n:
        return 'M';
    default:
        return '?'; // default unknown
    }
}

void DrawCurrentTime(const struct tm *timeinfo)
{
    char timestring[32], datestring[32];
    
    snprintf(timestring, 10, "%i:%02i", timeinfo->tm_hour > 12 ? timeinfo->tm_hour - 12 : (timeinfo->tm_hour == 0 ? 12 : timeinfo->tm_hour), timeinfo->tm_min);
    strftime(datestring, 32, "%A, %B %d", timeinfo);

    int16_t sx = 0, sy = 0;

    FillZone(superregion_zones, SUPERREGION_TIMEDATE, GxEPD_WHITE, false);
    DrawBorder(superregion_zones, SUPERREGION_TIMEDATE, GxEPD_BLACK);

    display.setTextColor(GxEPD_BLACK);

    GetAlignedStringCoordinatesInZone(timestring, currentTimeFont, nzones, CURRENT_TIME_ZONE, ALIGN_X_CENTER | ALIGN_Y_BOTTOM | Y_BOTTOM_PAD, &sx, &sy);
    display.setCursor(sx, sy);
    display.print(timestring);

    GetAlignedStringCoordinatesInZone(datestring, currentDateFont, nzones, CURRENT_DATE_ZONE, ALIGN_X_CENTER | ALIGN_Y_TOP, &sx, &sy);
    display.setCursor(sx, sy);
    display.print(datestring);
}

void DrawCurrentConditions(const currentWeather *current)
{
    //char icon = ConvertIconToChar(current->ico);

    char tempstring[32];
    int16_t sx = 0, sy = 0;

    FillZone(superregion_zones, SUPERREGION_CONDITIONS, GxEPD_WHITE, false);
    DrawBorder(superregion_zones, SUPERREGION_CONDITIONS, GxEPD_BLACK);

    display.setTextColor(GxEPD_BLACK);

    // snprintf(tempstring, 31, "%c", icon);
    // GetAlignedStringCoordinatesInZone(tempstring, currentConditionsFont, nzones, CONDITIONS_ZONE_TOP, ALIGN_X_CENTER | ALIGN_Y_BOTTOM | Y_BOTTOM_PAD, &sx, &sy);
    // display.setCursor(sx, sy);
    // display.print(tempstring);

    // snprintf(tempstring, 31, "%i", (int)(round(current->temp)));
    // GetAlignedStringCoordinatesInZone(tempstring, currentTempFont, nzones, CONDITIONS_ZONE_BOTTOM, ALIGN_X_CENTER | ALIGN_Y_TOP, &sx, &sy);
    // display.setCursor(sx, sy);
    // display.print(tempstring);

    snprintf(tempstring, 6, "%i/", (int)(round(current->temp)));
    GetAlignedStringCoordinatesInZone(tempstring, currentTempFont, nzones, ALT_CONDITIONS_TEMP, ALIGN_X_CENTER | ALIGN_Y_BOTTOM | Y_BOTTOM_PAD, &sx, &sy);
    display.setCursor(sx, sy);
    display.print(tempstring);

    // snprintf(tempstring, 31, "%s", current->description.c_str());
    // GetAlignedStringCoordinatesInZone(tempstring, currentDateFont, nzones, ALT_CONDITIONS_TIME, ALIGN_X_CENTER | ALIGN_Y_TOP, &sx, &sy);
    // display.setCursor(sx, sy);
    // display.print(tempstring);

}

void DrawHourForecast(hourlyWeather *data, int forecastZone, bool refresh = false)
{
    char tempstring[32];
    int16_t sx = 0, sy = 0;

    struct tm local_tm;
    if (TimestampToTzTimeInfo((time_t)data->time, &local_tm))
    {
        snprintf(tempstring, 10, "%i:%02i", local_tm.tm_hour > 12 ? local_tm.tm_hour - 12 : (local_tm.tm_hour == 0 ? 12 : local_tm.tm_hour), local_tm.tm_min);
        GetAlignedStringCoordinatesInZone(tempstring, forecastTimeFont, nzones, forecastZone + forcast_time_zone_offset, ALIGN_X_CENTER | ALIGN_Y_MIDDLE, &sx, &sy);
        display.setCursor(sx, sy);
        display.print(tempstring);
    }

    snprintf(tempstring, 31, "%i", (int)(round(data->temp)));
    GetAlignedStringCoordinatesInZone(tempstring, forecastTempFont, nzones, forecastZone + forcast_temp_zone_offset, ALIGN_X_CENTER | ALIGN_Y_MIDDLE, &sx, &sy);
    display.setCursor(sx, sy);
    display.print(tempstring);

    snprintf(tempstring, 31, "%c", ConvertIconToChar(data->ico));
    GetAlignedStringCoordinatesInZone(tempstring, forecastConditionFont, nzones, forecastZone + forcast_icon_zone_offset, ALIGN_X_CENTER | ALIGN_Y_MIDDLE, &sx, &sy);
    display.setCursor(sx, sy);
    display.print(tempstring);
}

// void DrawBatteryLevel(float battery_level, bool refresh = false)
// {
//     char tempstring[32];
//     int16_t sx = 0, sy = 0;
//     // warn about charging
//     if (battery_level <= 3.0)
//     {
//         GetAlignedStringCoordinatesInZone("Low Battery", &HPSimplified_Rg8pt7b, nzones, CURRENT_TIME_ZONE, ALIGN_X_CENTER | ALIGN_Y_TOP, &sx, &sy);
//         display.fillRect(0, 0, nzones[CURRENT_TIME_ZONE].w, sy + 8, GxEPD_BLACK);
//         display.setTextColor(GxEPD_WHITE);
//         display.setCursor(sx, sy + 2);
//         display.print("Low Battery");
//     }
//     else
//     {
//         // // current reading time
//         // temptimestamp = data->current.dt + data->timezone_offset;
//         // strftime(tempstring, 31, "%D %H:%M", localtime(&temptimestamp));
//         // GetAlignedStringCoordinatesInZone(tempstring, &DejaVu_Sans_Mono_10, nzones, STATUS_BAR_ZONE, ALIGN_X_LEFT | ALIGN_Y_MIDDLE, &sx, &sy);
//         // display.setCursor(sx, sy);
//         // display.print(tempstring);
//         snprintf(tempstring, 31, "%1.2fV", battery_level);
//         GetAlignedStringCoordinatesInZone(tempstring, &DejaVu_Sans_Mono_10, nzones, CURRENT_TIME_ZONE, ALIGN_X_CENTER | ALIGN_Y_TOP, &sx, &sy);
//         display.setCursor(sx, sy);
//         display.print(tempstring);
//     }
// }

void RefreshCurrentInformation(const currentWeather *current = nullptr, const struct tm *timeinfo = nullptr)
{
    if (current != nullptr && timeinfo != nullptr)
        SetWindow(superregion_zones, SUPERREGION_CURRENT, false);
    else if (current != nullptr)
        SetWindow(superregion_zones, SUPERREGION_CONDITIONS, false);
    else
        SetWindow(superregion_zones, SUPERREGION_TIMEDATE, false);

    // display.clearScreen();
    display.firstPage();
    display.setTextColor(GxEPD_BLACK);

    do
    {
        if (current != nullptr)
        {
            DrawCurrentConditions(current);
        }

        if (timeinfo != nullptr)
            DrawCurrentTime(timeinfo);

    } while (display.nextPage());
}

// void RefreshCurrentConditions(currentWeather *current)
// {
//     SetWindow(superregion_zones, SUPERREGION_CONDITIONS, false);

//     do
//     {
//         DrawCurrentConditions(current);
//     } while (display.nextPage());
// }

// void RefreshTimeAndDate()
// {
//     struct tm timeinfo;
//     char _time[32];
//     char _date[32];

//     if (getLocalTzTime(&timeinfo))
//     {
//         snprintf(_time, 10, "%i:%02i", timeinfo.tm_hour > 12 ? timeinfo.tm_hour - 12 : (timeinfo.tm_hour == 0 ? 12 : timeinfo.tm_hour), timeinfo.tm_min);
//         strftime(_date, 32, "%A, %B %d", &timeinfo);
//     }
//     else
//     {
//         snprintf(_time, 32, "??:??");
//         snprintf(_date, 32, "Can't get time.");
//     }

//     SetWindow(superregion_zones, SUPERREGION_TIMEDATE, false);

//     do
//     {
//         DrawCurrentTime(_time, _date);
//     } while (display.nextPage());
// }

void UpdateEntireDisplay(currentWeather *current, hourlyWeather *hourly)
{
    struct tm timeinfo;
    bool fetchedTime = false;
    char _time[32];
    char _date[32];

    fetchedTime = getLocalTzTime(&timeinfo);

    display.setPartialWindow(0, 0, display.width(), display.height());

    // display.clearScreen();
    display.firstPage();
    display.setTextColor(GxEPD_BLACK);

    do
    {
        APPDEBUG_PRINTLN("Displaying information");

        int numnzones = sizeof(nzones) / sizeof(Zone);
#ifdef REGIONBOXES
        for (int xz = 0; xz < numnzones; xz++)
            display.drawRect(nzones[xz].x, nzones[xz].y, nzones[xz].w, nzones[xz].h, GxEPD_BLACK);
#endif

        // draw border zones
        numnzones = sizeof(superregion_zones) / sizeof(Zone);
        for (int xz = 0; xz < numnzones; xz++)
        {
            DrawBorder(superregion_zones, xz, GxEPD_BLACK);
        }

        // ###############
        // ## current conditions
        // ###############
        if (fetchedTime)
            DrawCurrentTime(&timeinfo);

        DrawCurrentConditions(current);

        // ###############
        // ## forecast
        // ###############
        for (int forecastZone = 0; forecastZone < hourly_forecast_slots; forecastZone++)
        {
            DrawHourForecast(&hourly[forecastZone * 2], forecastZone);
        }

        // DrawBatteryLevel(battery_level);
    } while (display.nextPage());
}
