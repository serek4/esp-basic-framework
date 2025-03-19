#include "framework.hpp"

BasicConfig frameConfig(FRAMEWORK_CONFIG_NAME);
BasicWiFi wifi(WIFI_SSID, WIFI_PASS);
BasicOTA ota;
BasicWebServer webServer(WEB_SERVER_USER, WEB_SERVER_PASS);
BasicMqtt mqtt(MQTT_BROKER, MQTT_USER, MQTT_PASS);
BasicTime NTPclient(NTP_SERVER_ADDRESS, TIMEZONE);
BasicLogs logger;

Framework frame(&wifi, &ota, &mqtt, &webServer, &NTPclient, &logger, &frameConfig, LED_PIN, LED_ON);
AccessPoints accessPoints = WIFI_ACCESS_POINTS;

void handleWiFiConnected(CONNECTED_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onConnected");
}
void handleWiFiGotIP(GOT_IP_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onGotIP " + (WiFi.localIP()).toString());
}
void handleWiFiDisconnected(DISCONNECTED_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onDisconnected");
}

void blink(u_long onTime, u_long offTime) {
	frame.blinkLed(onTime, offTime);
}

void handleMqttConnect(bool sessionPresent) {
	Serial.println("User handler for MQTT onConnect");
}
void handleMqttPublish(PacketID packetId) {
	Serial.printf("Packet: %i successfully published\n", packetId);
}
void handleMqttDisconnect(espMqttClientTypes::DisconnectReason reason) {
	Serial.println("User handler for MQTT onDisconnect");
}
void handleIncMqttMsg(const char* topic, const char* payload) {
	Serial.printf("Incoming mqtt message!\n msg.topic:   %s\n msg.payload: %s\n", topic, payload);
	mqtt.publish((mqtt.topicPrefix + "/feedback").c_str(), payload);
}
bool handleMqttCommands(BasicMqtt::Command mqttCommand) {
	std::string command = "";
	for (const auto& element : mqttCommand) {
		command += element;
		command += " ";
	}
	command.pop_back();
	Serial.printf("Incoming mqtt command: %s\n", command.c_str());
	return true;
}

void Framework::setup() {
	EspBasic::_setup();
	webServer.setup();
	mqtt.onConnect(handleMqttConnect);
	mqtt.onPublish(handleMqttPublish);
	mqtt.onDisconnect(handleMqttDisconnect);
	mqtt.onMessage(handleIncMqttMsg);
	mqtt.commands(handleMqttCommands);
	mqtt.setWaitingFunction(blink);
	mqtt.setup();
	wifi.setAccessPoints(accessPoints);
	wifi.onConnected(handleWiFiConnected);
	wifi.onGotIP(handleWiFiGotIP);
	wifi.onDisconnected(handleWiFiDisconnected);
	wifi.setWaitingFunction(blink);
	wifi.setup();
	if (wifi.waitForConnection() == BasicWiFi::wifi_got_ip) {
		NTPclient.waitForNTP();
		mqtt.waitForConnection();
	}
	EspBasic::setupDone();    //* here or in main setup() end
}

void Framework::loop() {
	EspBasic::_loop();
}
