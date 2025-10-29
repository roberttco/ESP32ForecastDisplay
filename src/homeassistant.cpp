#include "config.h"

#include <Arduino.h>
#include <HttpClient.h>

#include "secret.h"

String GetHaState(String entity_id, String *lastChanged)
{
    String rval = "";

    HTTPClient http_client;

    String url = HOME_ASSISTANT_PROTOCOL;
    url += "://";
    url += HOME_ASSISTANT_SERVER;
    url += ":";
    url += HOME_ASSISTANT_PORT;
    url += "/api/states/";
    url += entity_id;

    #ifdef APPDEBUG
    Serial.printf("Getting HA state from %s\n", url.c_str());
    #endif

    http_client.begin(url);
    http_client.addHeader("Authorization", HOME_ASSISTANT_AUTH);
    http_client.setTimeout(5000);
    int httpCode = http_client.GET();

    String payload;

    // httpCode will be negative on error
    if (httpCode > 0) {
        // file found at server
        if (httpCode == HTTP_CODE_OK) {
            payload = http_client.getString();
        } 
#ifndef APPDEBUG
    }
#else
        else {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        }
    }
    else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http_client.errorToString(httpCode).c_str());
    }
#endif

    http_client.end();

    if (httpCode == HTTP_CODE_OK)
    {
        // now parse the payload looking for "content":
        int labelStart = payload.indexOf("state\":");
        // find the first { after "content":
        int contentStart = labelStart + 7;
        // find the following } and get what's between the braces:
        int contentEnd = payload.indexOf("\"", contentStart + 1);
        String content = payload.substring(contentStart + 1, contentEnd);

        labelStart = payload.indexOf("last_changed\":",contentEnd);
        contentStart = labelStart + 14;
        contentEnd = payload.indexOf("\"", contentStart + 1);
        String last_changed_str = payload.substring(contentStart + 1, contentEnd);
        if (lastChanged != nullptr)
        {
            *lastChanged = last_changed_str;
        }

        #ifdef APPDEBUG
        Serial.printf("Result:\n     state = %s\n     last_changed = %s\n", content.c_str(), last_changed_str.c_str());
        #endif

        rval = content;
    }

    APPDEBUG_PRINT("GetHaState return val: ");
    APPDEBUG_PRINTLN(rval);

    return rval;
}

float GetTemperature(String * lastChanged)
{
    float rval = -99;
    String last_changed;
    String state_val = GetHaState("sensor.environment_sensor_0f95eb_temperature",&last_changed);

    if (state_val.isEmpty() == false)
    {
        rval = state_val.toFloat();

        if (lastChanged != nullptr)
        {
            *lastChanged = last_changed;
        }
    }

    return rval;
}
