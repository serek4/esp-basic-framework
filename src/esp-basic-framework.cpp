#include "esp-basic-framework.hpp"
EspBasic::EspBasic(BasicWiFi* wifi, BasicOTA* ota, BasicMqtt* mqtt, BasicWebServer* webServer,
                   BasicTime* NTPclient, BasicLogs* logger, BasicConfig* config,
                   uint8_t ledPin, bool ONstate, bool useLed)
    : _useLed(useLed)
    , _ledPin(ledPin)
    , _ledON(ONstate)
    , _reboot(rbt_idle)
    , _1minTimer(0)
    , _1secTimer(0)
    , _prevLoopTime(0)
    , _loopCount(0)
    , _avgLoopBuffer(0)
#ifdef ARDUINO_ARCH_ESP32
    , _avgTemperatureCounter(0)
    , _avgTemperatureBuffer(0)
#endif
    , loopTime(0)
    , avgLoopTime(0)
    , loopCount(0)
    , avgLoopBuffer(0)
#ifdef ARDUINO_ARCH_ESP32
    , avgTemperature(0)
#endif
    , _wifi(wifi)
    , _ota(ota)
    , _mqtt(mqtt)
    , _webServer(webServer)
    , _NTPclient(NTPclient)
    , _logger(logger)
    , _config(config) {
}
EspBasic::EspBasic(BasicWiFi* wifi, BasicOTA* ota, BasicMqtt* mqtt, BasicWebServer* webServer,
                   BasicTime* NTPclient, BasicLogs* logger, BasicConfig* config)
    : EspBasic::EspBasic(wifi, ota, mqtt, webServer, NTPclient, logger, config,
                         255, LOW, false) {
}
EspBasic::EspBasic(uint8_t ledPin, bool ONstate)
    : EspBasic::EspBasic(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                         ledPin, ONstate, true) {
}
EspBasic::EspBasic()
    : EspBasic::EspBasic(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                         255, LOW, false) {
}

void EspBasic::blinkLed(u_long onTime, u_long offTime, uint8_t repeat) {
	for (int i = 0; i < repeat; i++) {
		digitalWrite(_ledPin, _ledON);
		delay(onTime);
		digitalWrite(_ledPin, !_ledON);
		delay(offTime);
	}
}

void EspBasic::_shutdown() {
	_mqtt->disconnect();
	while (_mqtt->connected()) { delay(1); }
	_wifi->disconnect();
	while (WiFi.isConnected()) { delay(1); }
	if (_logger != nullptr) { _logger->saveLog(BasicLogs::_info_, "restart"); }
}

uint16_t EspBasic::_loopTime() {
	u_long microsNow = micros();
	loopTime = microsNow - _prevLoopTime;
	_prevLoopTime = microsNow;
	if (_loopCount == UINT32_MAX || _avgLoopBuffer >= UINT32_MAX - (loopTime * 2)) {
		_loopCount = 0;
		_avgLoopBuffer = 0;
	}
	_loopCount++;
	_avgLoopBuffer += loopTime;
	return loopTime;
}
uint16_t EspBasic::_avgLoopTime() {
	avgLoopTime = static_cast<uint16_t>(_avgLoopBuffer / _loopCount);
	avgLoopBuffer = _avgLoopBuffer;
	loopCount = _loopCount;
	_avgLoopBuffer = 0;
	_loopCount = 0;
	return avgLoopTime;
}
#ifdef ARDUINO_ARCH_ESP32
float EspBasic::_avgTemperature() {
	if (_avgTemperatureCounter > 0) {
		avgTemperature = static_cast<float>(_avgTemperatureBuffer / _avgTemperatureCounter) / 10.0F;
		_avgTemperatureCounter = 0;
		_avgTemperatureBuffer = 0;
	} else {
		avgTemperature = _internalTemperature();
	}
	return avgTemperature;
}
#endif
void EspBasic::setupDone() {
	// loop timers boot + setup() offset
	_1minTimer = millis();
	_prevLoopTime = micros();    // save setup end time as first loop start
}

void EspBasic::_setup() {
#ifdef ARDUINO_ARCH_ESP32
	avgTemperature = _internalTemperature();
#endif
	if (_useLed) {
		pinMode(_ledPin, OUTPUT);
		digitalWrite(_ledPin, _ledON);
	}
	filesystem.setup(true);
	if (_config != nullptr) {
		_config->addLogger(&BasicLogs::saveLog);
		_config->setup();
		if (_wifi != nullptr) {
			_config->serialize([&](JsonObject doc) {
				BasicWiFi::Config wifiConfig = _wifi->getConfig();
				JsonObject wifi = doc["wifi"].as<JsonObject>();
				if (wifi.isNull()) { wifi = doc["wifi"].to<JsonObject>(); }
				wifi["ssid"] = wifiConfig.ssid;
				wifi["pass"] = wifiConfig.pass;
				if (wifiConfig.ip != NULL_IP_ADDR && wifiConfig.subnet != NULL_IP_ADDR) {
					wifi["ip"] = wifiConfig.ip.toString();
					wifi["subnet"] = wifiConfig.subnet.toString();
					wifi["gateway"] = wifiConfig.gateway.toString();
					wifi["dns1"] = wifiConfig.dns1.toString();
					wifi["dns2"] = wifiConfig.dns2.toString();
				}
			});
			_config->deserialize([&](JsonObject doc) {
				BasicWiFi::Config wifiConfig = _wifi->getConfig();
				JsonObject wifi = doc["wifi"].as<JsonObject>();
				if (!wifi.isNull()) {
					if (!wifi["ssid"].isNull()) { wifiConfig.ssid = wifi["ssid"].as<String>(); }
					if (!wifi["pass"].isNull()) { wifiConfig.pass = wifi["pass"].as<String>(); }
					if (!wifi["ip"].isNull() && !wifi["subnet"].isNull()) {
						wifiConfig.ip.fromString(wifi["ip"].as<const char*>());
						wifiConfig.subnet.fromString(wifi["subnet"].as<const char*>());
						if (!wifi["gateway"].isNull()) { wifiConfig.gateway.fromString(wifi["gateway"].as<const char*>()); }
						if (!wifi["dns1"].isNull()) { wifiConfig.dns1.fromString(wifi["dns1"].as<const char*>()); }
						if (!wifi["dns2"].isNull()) { wifiConfig.dns2.fromString(wifi["dns2"].as<const char*>()); }
					}
				}
				_wifi->setConfig(wifiConfig);
			});
		}
		if (_webServer != nullptr) {
			_config->serialize([&](JsonObject doc) {
				BasicWebServer::Config webServerConfig = _webServer->getConfig();
				JsonObject http = doc["http"].as<JsonObject>();
				if (http.isNull()) { http = doc["http"].to<JsonObject>(); }
				http["user"] = webServerConfig.user;
				http["pass"] = webServerConfig.pass;
			});
			_config->deserialize([&](JsonObject doc) {
				BasicWebServer::Config webServerConfig;
				_webServer->getConfig(webServerConfig);
				JsonObject http = doc["http"].as<JsonObject>();
				if (!http.isNull()) {
					if (!http["user"].isNull()) { webServerConfig.user = http["user"].as<String>(); }
					if (!http["pass"].isNull()) { webServerConfig.pass = http["pass"].as<String>(); }
				}
				_webServer->setConfig(webServerConfig);
			});
		}
		if (_mqtt != nullptr) {
			_config->serialize([&](JsonObject doc) {
				BasicMqtt::Config mqttConfig = _mqtt->getConfig();
				JsonObject mqtt = doc["mqtt"].as<JsonObject>();
				if (mqtt.isNull()) { mqtt = doc["mqtt"].to<JsonObject>(); }
				mqtt["broker"] = mqttConfig.broker_address;
				mqtt["user"] = mqttConfig.user;
				mqtt["pass"] = mqttConfig.pass;
			});
			_config->deserialize([&](JsonObject doc) {
				BasicMqtt::Config mqttConfig = _mqtt->getConfig();
				JsonObject mqtt = doc["mqtt"].as<JsonObject>();
				if (!mqtt.isNull()) {
					if (!mqtt["broker"].isNull()) { mqttConfig.broker_address = mqtt["broker"].as<std::string>(); }
					if (!mqtt["user"].isNull()) { mqttConfig.user = mqtt["user"].as<std::string>(); }
					if (!mqtt["pass"].isNull()) { mqttConfig.pass = mqtt["pass"].as<std::string>(); }
				}
				_mqtt->setConfig(mqttConfig);
			});
		}
		_config->load();
	}
	if (_webServer != nullptr) {
		_webServer->addHttpHandler("/restart", [&](AsyncWebServerRequest* request) {
			if (_logger != nullptr) { _logger->saveLog("http", "restart requested"); }
			AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "restart command sent");
			request->send(response);
			_reboot = rbt_forced;
		});
		_webServer->addHttpHandler("/reboot", [&](AsyncWebServerRequest* request) {
			if (_logger != nullptr) { _logger->saveLog("http", "reboot requested"); }
			AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "reboot command sent");
			request->send(response);
			_reboot = rbt_requested;
		});
		if (_wifi != nullptr) {
			_webServer->addHttpHandler("/reconnectWiFi", [&](AsyncWebServerRequest* request) {
				if (_logger != nullptr) { _logger->saveLog("http", "WiFi reconnect requested"); }
				AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "reconnect WiFi command sent");
				request->send(response);
				_wifi->reconnect();
			});
		}
		if (_mqtt != nullptr) {
			_webServer->addHttpHandler("/reconnectMqtt", [&](AsyncWebServerRequest* request) {
				if (_logger != nullptr) { _logger->saveLog("http", "MQTT reconnect requested"); }
				AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "reconnect mqtt command sent");
				request->send(response);
				_mqtt->reconnect();
			});
		}
		if (_NTPclient != nullptr) {
			_webServer->addHttpHandler("/syncTime", [&](AsyncWebServerRequest* request) {
				if (_logger != nullptr) { _logger->saveLog("http", "NTP sync requested"); }
				AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "NTP sync request sent");
				request->send(response);
				BasicTime::requestNtpTime();
			});
		}
	}
	if (_mqtt != nullptr) {
		if (_logger != nullptr) { _mqtt->addLogger(&BasicLogs::saveLog); }
		_mqtt->onConnect([&](bool sessionPresent) {
			_publishStats();
		});
		_mqtt->commands([&](BasicMqtt::Command mqttCommand) {
			if (mqttCommand[0] == "restart" && mqttCommand.size() == 1) {
				if (_logger != nullptr) { _logger->saveLog("mqtt", "restart requested"); }
				_reboot = rbt_forced;
				return true;
			}
			if (mqttCommand[0] == "reboot" && mqttCommand.size() == 1) {
				if (_logger != nullptr) { _logger->saveLog("mqtt", "reboot requested"); }
				_reboot = rbt_requested;
				return true;
			}
			if (mqttCommand[0] == "wifi") {
				if (mqttCommand.size() > 1) {
					if (mqttCommand[1] == "reconnect") {
						if (_logger != nullptr) { _logger->saveLog("mqtt", "WiFi reconnect requested"); }
						_wifi->reconnect();
						return true;
					}
				}
			}
			if (_config != nullptr) {
				if (mqttCommand[0] == "config") {
					if (mqttCommand.size() > 1) {
						if (mqttCommand[1] == "save") {    // to file
							if (_logger != nullptr) { _logger->saveLog("mqtt", "saving config"); }
							_config->save();
							return true;
						}
						if (mqttCommand[1] == "load") {    // from file
							if (_logger != nullptr) { _logger->saveLog("mqtt", "loading config"); }
							_config->load();
							return true;
						}
					}
				}
			}
			return false;
		});
	}
	if (_wifi != nullptr) {
		if (_logger != nullptr) { _wifi->addLogger(&BasicLogs::saveLog); }
		_wifi->onGotIP([&](GOT_IP_HANDLER_ARGS) {
			if (_ota != nullptr) { _ota->begin(); }
			if (_mqtt != nullptr) { _mqtt->connect(); }
			if (_webServer != nullptr) { _webServer->begin(); }
		});
		_wifi->onDisconnected([&](DISCONNECTED_HANDLER_ARGS) {
			if (_mqtt != nullptr) { _mqtt->disconnect(); }
		});
	}
	if (_ota != nullptr) {
		if (_logger != nullptr) { _ota->addLogger(&BasicLogs::saveLog); }
		_ota->setup();
	}
	if (_NTPclient != nullptr) {
		if (_logger != nullptr) { _NTPclient->addLogger(&BasicLogs::saveLog); }
		_NTPclient->setup();
	}
}
void EspBasic::_loop() {
	if (_NTPclient != nullptr) { _NTPclient->handle(); }
	if (_ota != nullptr) { _ota->handle(); }
	_loopTime();
	if (millis() - _1secTimer >= ONE_SECOND) {
		_1secTimer = millis();
#ifdef ARDUINO_ARCH_ESP32
		_avgTemperatureCounter++;
		_avgTemperatureBuffer += static_cast<uint32_t>(_internalTemperature() * 10);
#endif
	}
	if (millis() - _1minTimer >= ONE_MINUTE) {
		_1minTimer = millis();
		now();
		_avgLoopTime();
#ifdef ARDUINO_ARCH_ESP32
		_avgTemperature();
#endif
		_publishStats();
	}
	if (_logger != nullptr) { _logger->handle(); }
	if (_reboot != rbt_idle) {
		if (_reboot == rbt_requested) {
			_shutdown();
			_reboot = rbt_pending;
		}
		static u_long rebootTimer = millis();
		if (millis() - rebootTimer > REBOOT_DELAY || _reboot == rbt_forced) {
			rebootTimer = millis();
			ESP.restart();
		}
	}
}

void EspBasic::_publishStats() {
	if (_mqtt != nullptr) {
		_mqtt->publish((_mqtt->topicPrefix + "/rssi").c_str(), _wifiRssi());
		_mqtt->publish((_mqtt->topicPrefix + "/cpu_freq").c_str(), _cpuFrequency());
#ifdef ARDUINO_ARCH_ESP32
		_mqtt->publish((_mqtt->topicPrefix + "/temperature").c_str(), avgTemperature);
#endif
		JsonDocument doc;
		String loopStats;
		if (loopTime > 0) {
			doc["time"] = loopTime;
			doc["avgTime"] = avgLoopTime;
			doc["count"] = loopCount;
			doc["buffer"] = avgLoopBuffer;
			serializeJson(doc, loopStats);
			_mqtt->publish((_mqtt->topicPrefix + "/loop").c_str(), loopStats);
			doc.clear();
		}
		String heapStats;
		doc["free"] = _heapFree();
#ifdef ARDUINO_ARCH_ESP32
		doc["minFree"] = _heapMinFree();
#elif defined(ARDUINO_ARCH_ESP8266)
		doc["stackFree"] = _stackFree();
#endif
		doc["maxAlloc"] = _heapMaxAlloc();
		serializeJson(doc, heapStats);
		_mqtt->publish((_mqtt->topicPrefix + "/heap").c_str(), heapStats);
#if defined(ARDUINO_ARCH_ESP32) && defined(BOARD_HAS_PSRAM)
		doc.clear();
		String PsramStats;
		doc["free"] = ESP.getFreePsram();
		doc["minFree"] = ESP.getMinFreePsram();
		doc["maxAlloc"] = ESP.getMaxAllocPsram();
		serializeJson(doc, PsramStats);
		_mqtt->publish((_mqtt->topicPrefix + "/psram").c_str(), PsramStats);
#endif
	}
}
