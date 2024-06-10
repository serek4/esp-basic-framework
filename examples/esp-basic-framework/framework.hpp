#pragma once

#include <esp-basic-framework.h>

#define LED_PIN 2
#define LED_ON HIGH

#define FRAMEWORK_CONFIG_NAME "frameworkConfig"

#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"
// clang-format off
#define WIFI_ACCESS_POINT_1 { "FF:FF:FF:FF:FF:FF", "AP1" }
#define WIFI_ACCESS_POINT_2 { "EE:EE:EE:EE:EE:EE", "AP2" }
#define WIFI_ACCESS_POINT_3 { "DD:DD:DD:DD:DD:DD", "AP3" }
#define WIFI_ACCESS_POINTS { WIFI_ACCESS_POINT_1, WIFI_ACCESS_POINT_2, WIFI_ACCESS_POINT_3 }
// clang-format on
#define WEB_SERVER_USER "admin"
#define WEB_SERVER_PASS "admin"
#define MQTT_BROKER "mqtt-broker.lan"
#define MQTT_USER "user"
#define MQTT_PASS "password"
#define NTP_SERVER_ADDRESS "pool.ntp.org"
#define TIMEZONE 1    // Central European Time (Europe/Warsaw)

#include "secrets.h"

class Framework : public EspBasic {
  private:
  public:
	using EspBasic::EspBasic;

	void setup();
	void loop();
};

extern Framework frame;
// extern BasicConfig frameConfig;
extern BasicWiFi wifi;
// extern BasicOTA ota;
// extern BasicWebServer webServer;
extern BasicMqtt mqtt;
// extern BasicTime NTPclient;
// extern BasicLogs logger;
