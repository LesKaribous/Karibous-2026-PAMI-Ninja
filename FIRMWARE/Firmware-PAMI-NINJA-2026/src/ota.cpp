#ifdef OTA_ENABLED

#include <WiFi.h>
#include <ArduinoOTA.h>
#include "ota_credentials.h"

static void otaTask(void*)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
        vTaskDelay(pdMS_TO_TICKS(100));

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA] WiFi failed");
        vTaskDelete(nullptr);
        return;
    }

    Serial.print("[OTA] Connected, IP: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname("pami-ninja");
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();
    Serial.println("[OTA] Ready");

    for (;;) {
        ArduinoOTA.handle();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void initOTA()
{
    xTaskCreatePinnedToCore(otaTask, "OTA", 10240, nullptr, 4, nullptr, 0);
}

#endif
