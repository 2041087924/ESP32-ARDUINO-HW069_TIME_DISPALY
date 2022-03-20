#include "wifi_config.hpp"
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

DNSServer dnsServer;
DynamicJsonDocument doc(1024);
int8_t default_config_index = 0, config_list_len = 0;
bool get_config(const char *path)
{
    default_config_index = 0;
    config_list_len = 0;
    SPIFFS.begin();
    if (!SPIFFS.exists(path))
        return false;
    File config_file_streamer = SPIFFS.open(path);
    if (0 == config_file_streamer)
        return false;
    DeserializationError error = deserializeJson(doc, config_file_streamer);
    if ((DeserializationError::Ok != error) || (doc.isNull()))
        return false;
    if (!doc.containsKey("default_config") || !doc.containsKey("config_list_len") || !doc.containsKey("config_list"))
        return false;
    default_config_index = doc["default_config"];
    config_list_len = doc["config_list_len"];
    if ((default_config_index < 0) || (config_list_len <= 0))
    {
        default_config_index = 0;
        config_list_len = 0;
        return false;
    }
    return true;
}

bool try_connect_to(const char *_ssid, const char *_password)
{
    if (NULL == _ssid || NULL == _password || 0 == strlen(_ssid) || 32 < strlen(_ssid) || 0 == strlen(_password) || 64 < strlen(_password))
        return false;
    Serial.print("\ntry connect to: ");
    Serial.println(_ssid);
    Serial.print("\nwith password: ");
    Serial.println(_password);
    uint8_t wait_time = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    while (!WiFi.isConnected())
    {
        wait_time++;
        Serial.print(".");
        delay(1000);
        if (wait_time > 15)
        {
            Serial.printf("\nWiFi Failed!\n");
            return false;
        }
        else if (5 == wait_time || 10 == wait_time)
        {
            WiFi.reconnect();
            Serial.printf("\nWiFi Failed!, retry\n");
        }
    }
    return true;
}

bool connect_to_wifi(const char *path)
{
    bool ret;
    if (!WiFi.isConnected())
    {
        if (get_config(path))
        {
            const char *ssid;
            const char *password;
            default_config_index = (default_config_index < config_list_len) ? default_config_index : 0;
            ssid = doc["config_list"][default_config_index]["config"]["ssid"];
            password = doc["config_list"][default_config_index]["config"]["password"];
            if (!try_connect_to(ssid, password))
            {
                for (uint8_t i = 0; i < config_list_len; i++)
                {
                    if (default_config_index == i)
                        continue;
                    ssid = doc["config_list"][i]["config"]["ssid"];
                    password = doc["config_list"][i]["config"]["password"];
                    if (try_connect_to(ssid, password))
                        break;
                }
            }
        }
    }

    if (WiFi.isConnected())
    {
        Serial.print("\nConnected to ");
        Serial.println(WiFi.SSID());
        Serial.print("\nIP address: ");
        Serial.println(WiFi.localIP());
        ret = true;
    }
    else
    {
        if (WiFi.softAP("esp-captive"))
        {
            dnsServer.start(53, "*", WiFi.softAPIP());
            Serial.print("\nssid : ");
            Serial.println(WiFi.softAPSSID());
            Serial.print("\nIP address: ");
            Serial.println(WiFi.softAPIP());
            ret = true;
        }
        else
            ret = false;
    }
    return ret;
}
