#include "framework.hpp"

BasicConfig frameConfig(FRAMEWORK_CONFIG_NAME);
BasicWiFi wifi(WIFI_SSID, WIFI_PASS);
BasicOTA ota;
BasicWebServer webServer(WEB_SERVER_USER, WEB_SERVER_PASS);
BasicMqtt mqtt(MQTT_BROKER, MQTT_USER, MQTT_PASS);
BasicTime NTPclient(NTP_SERVER_ADDRESS, TIMEZONE);
BasicLogs logger;

Framework frame(LED_PIN, LED_ON);
AccessPoints accessPoints = WIFI_ACCESS_POINTS;

void serializeWifiConfig(JsonObject doc) {
	BasicWiFi::Config wifiConfig = wifi.getConfig();
	doc["wifi"]["ssid"] = wifiConfig.ssid;
	doc["wifi"]["pass"] = wifiConfig.pass;
}
void serializeWebServerConfig(JsonObject doc) {
	BasicWebServer::Config webServerConfig = webServer.getConfig();
	JsonObject http = doc["http"].to<JsonObject>();
	http["user"] = webServerConfig.user;
	http["pass"] = webServerConfig.pass;
}
void serializeMqttConfig(JsonObject doc) {
	BasicMqtt::Config mqttConfig = mqtt.getConfig();
	JsonObject _mqtt = doc["mqtt"].to<JsonObject>();
	_mqtt["broker"] = mqttConfig.broker_address;
	_mqtt["user"] = mqttConfig.user;
	_mqtt["pass"] = mqttConfig.pass;
}

void deserializeWiFiConfig(JsonObject doc) {
	BasicWiFi::Config wifiConfig;
	wifi.getConfig(wifiConfig);
	wifiConfig.ssid = doc["wifi"]["ssid"].as<String>();
	wifiConfig.pass = doc["wifi"]["pass"].as<String>();
	wifi.setConfig(wifiConfig);
}
void deserializeWebServerConfig(JsonObject doc) {
	BasicWebServer::Config webServerConfig;
	webServer.getConfig(webServerConfig);
	JsonObject http = doc["http"];
	if (!http.isNull()) {
		webServerConfig.user = http["user"].as<String>();
		webServerConfig.pass = http["pass"].as<String>();
	}
	webServer.setConfig(webServerConfig);
}
void deserializeMqttConfig(JsonObject doc) {
	BasicMqtt::Config mqttConfig = mqtt.getConfig();
	JsonObject _mqtt = doc["mqtt"];
	if (!_mqtt.isNull()) {
		mqttConfig.broker_address = _mqtt["broker"].as<std::string>();
		mqttConfig.user = _mqtt["user"].as<std::string>();
		mqttConfig.pass = _mqtt["pass"].as<std::string>();
	}
	mqtt.setConfig(mqttConfig);
}

void handleWiFiConnected(CONNECTED_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onConnected");
}
void handleWiFiGotIP(GOT_IP_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onGotIP");
	webServer.begin();
}
void handleWiFiDisconnected(DISCONNECTED_HANDLER_ARGS) {
	Serial.println("User handler for WIFI onDisconnected");
}

void blink(u_long onTime, u_long offTime) {
	frame.blinkLed(onTime, offTime);
}

void httpReconnectWiFi(AsyncWebServerRequest* request) {
	BasicLogs::saveLog(now(), BasicLogs::_info_, "manual WiFi reconnect");
	AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "reconnect WiFi command sent");
	request->send(response);
	wifi.reconnect();
}
void httpReconnectMqtt(AsyncWebServerRequest* request) {
	BasicLogs::saveLog(now(), BasicLogs::_info_, "manual MQTT reconnect");
	AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "reconnect mqtt command sent");
	request->send(response);
	mqtt.reconnect();
}
void httpSyncTime(AsyncWebServerRequest* request) {
	BasicLogs::saveLog(now(), BasicLogs::_info_, "manual NTP sync");
	AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "NTP sync request sent");
	request->send(response);
	BasicTime::requestNtpTime();
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
	if (mqttCommand[0] == "config") {
		if (mqttCommand.size() > 1) {
			if (mqttCommand[1] == "save") {    // to file
				Serial.println("saving config");
				frameConfig.save();
				return true;
			}
			if (mqttCommand[1] == "load") {    // from file
				Serial.println("loading config");
				frameConfig.load();
				return true;
			}
		}
	}
	return false;
}

void Framework::setup() {
	EspBasic::_wifi = &wifi;
	EspBasic::_ota = &ota;
	EspBasic::_mqtt = &mqtt;
	EspBasic::_setup();
	filesystem.setup(true);
	frameConfig.addLogger(&BasicLogs::saveLog);
	frameConfig.setup();
	frameConfig.serialize(serializeWifiConfig);
	frameConfig.serialize(serializeWebServerConfig);
	frameConfig.serialize(serializeMqttConfig);
	frameConfig.deserialize(deserializeWiFiConfig);
	frameConfig.deserialize(deserializeWebServerConfig);
	frameConfig.deserialize(deserializeMqttConfig);
	frameConfig.load();
	webServer.addHttpHandler("/reconnectWiFi", httpReconnectWiFi);
	webServer.addHttpHandler("/reconnectMqtt", httpReconnectMqtt);
	webServer.addHttpHandler("/syncTime", httpSyncTime);
	webServer.setup();
	mqtt.addLogger(&BasicLogs::saveLog);
	mqtt.onConnect(handleMqttConnect);
	mqtt.onPublish(handleMqttPublish);
	mqtt.onDisconnect(handleMqttDisconnect);
	mqtt.onMessage(handleIncMqttMsg);
	mqtt.commands(handleMqttCommands);
	mqtt.setWaitingFunction(blink);
	mqtt.setup();
	wifi.setAccessPoints(accessPoints);
	wifi.addLogger(&BasicLogs::saveLog);
	wifi.onConnected(handleWiFiConnected);
	wifi.onGotIP(handleWiFiGotIP);
	wifi.onDisconnected(handleWiFiDisconnected);
	wifi.setWaitingFunction(blink);
	wifi.setup();
	NTPclient.addLogger(&BasicLogs::saveLog);
	NTPclient.setup();
	ota.addLogger(&BasicLogs::saveLog);
	if (wifi.waitForConnection() == BasicWiFi::wifi_got_ip) {
		NTPclient.waitForNTP();
		mqtt.waitForConnection();
	}
	EspBasic::setupDone();    //* here or in main setup() end
}

void Framework::loop() {
	NTPclient.handle();
	logger.handle();
	EspBasic::_loop();
	if (millis() - _1minTimer >= ONE_MINUTE) {
		_1minTimer = millis();
		now();
		EspBasic::_avgLoopTime();
		EspBasic::_publishStats();
	}
}
