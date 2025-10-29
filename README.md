# WeatherForecastDisplay
This code applies to the [XIAO 7.5" ePaper Panel](https://www.seeedstudio.com/XIAO-7-5-ePaper-Panel-p-6416.html) and shows the time, current temperature, and a 12-hour forecast.  The time is retrieved form the esp32 RTC and the current temperature is retrieved via URL (e.g. from home Assistant).  The forecast is pulled form a Open Weather One Call API invocation.

## Attribution
The code utilizes the following libraries (see also platformio.ini):

|Library|Author|URL|
|---|---|---|
| GxEPD2 | Jean-Marc Zingg | https://github.com/ZinggJM/GxEPD2 |
| Adafruit GFX | Adafruit | https://github.com/adafruit/Adafruit-GFX-Library |
| OWMOneCall | Bill Wilhelm | https://github.com/bilhelm96/OWMOneCall |
| Arduino JSON | Benoit Blanchon | https://arduinojson.org/ |
| Timezone | Jack Christensen |https://github.com/JChristensen/Timezone |

## Resources

* [XIAO 7.5" ePaper Panel product](https://www.seeedstudio.com/XIAO-7-5-ePaper-Panel-p-6416.html)
* [XIAO 7.5" ePaper Panel Wiki](https://wiki.seeedstudio.com/xiao_075inch_epaper_panel/)
* [XIAO 7.5" ePaper Panel Schematic](https://files.seeedstudio.com/wiki/xiao_075inch_epaper_panel/ePaper_Driver_Board.pdf)
* [XIAO ESP32-C3](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
* [XIAO ESP32-C3 Schematic](https://files.seeedstudio.com/wiki/XIAO_WiFi/Resources/Seeeduino-XIAO-ESP32C3-SCH.pdf)


## Configuration
The include/config.h file defines a few configuration values.

|Configuration|Description|
|---|---|
|MY_LATITUDE|Your latitude (as accurate as possible in decimal notation)|
|MY_LONGITUDE|Your longitude (as accurate as possible in decimal notation)|
|FORECAST_INTERVAL_MINUTES|How often to update the 12 hour forecast in seconds|
|TEMPERATURE_INTERVAL_MINUTES|How often to update the current temperature in minutes|


## Secrets
The code requires a few secret values.  These are stored in the include/secret.h file (ignored in gitignore to avoid comitting to the repo).  The values are:

|Secret|Description|
|---|---|
|WIFI_SSID | Your WiFi SSID|
|WIFI_PASSWORD | Your WiFi password|
|OPEN_WEATHER_MAP_APP_ID |[Your Open Weather Map API Key](https://home.openweathermap.org/users/sign_up)|
|HOME_ASSISTANT_AUTH|Home assistant bearer token tog et current temperature (see below)|
|HOME_ASSISTANT_SERVER|Hostname or IP address of the Home Assistant server|
|HOME_ASSISTANT_PORT|Home Assistant port (e.g. 8123)|
|HOME_ASSISTANT_PROTOCOL|Either *http* or *https*|

## Current temperature
The current temperature is retrieved from HA.  It is implemented this way to avoid doing a full OWM call once every update interval to save battery power and to not exceed the OWM API call limit.

## Current time
The time is retrieved via NTP at initial power on and every time the 12 hour forecast is updated.  The onboard RTC is updated with the NTP time.  Once per minte, the time on the display is updated to reflect the RTC time.

## Other
Pressing and holding the BOOT button on the back of the display will prevent deep sleep.  This is useful for loading new code.
