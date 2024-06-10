#include "esp-basic-framework.hpp"

EspBasic::EspBasic(uint8_t ledPin, bool ONstate, bool useLed)
    : _useLed(useLed)
    , _ledPin(ledPin)
    , _ledON(ONstate)
    , _1minTimer(0)
    , _prevLoopTime(0)
    , _loopCount(0)
    , _avgLoopBuffer(0)
    , loopTime(0)
    , avgLoopTime(0)
    , loopCount(0)
    , avgLoopBuffer(0)
    , _wifi(nullptr)
    , _ota(nullptr)
    , _mqtt(nullptr)
    , _webServer(nullptr)
    , _NTPclient(nullptr) {
}
EspBasic::EspBasic()
    : EspBasic::EspBasic(255, LOW, false) {
}

void EspBasic::blinkLed(u_long onTime, u_long offTime, uint8_t repeat) {
	for (int i = 0; i < repeat; i++) {
		digitalWrite(_ledPin, _ledON);
		delay(onTime);
		digitalWrite(_ledPin, !_ledON);
		delay(offTime);
	}
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
void EspBasic::setupDone() {
	// loop timers boot + setup() offset
	_1minTimer = millis();
	_prevLoopTime = micros();    // save setup end time as first loop start
}

void EspBasic::_setup() {
	if (_useLed) {
		pinMode(_ledPin, OUTPUT);
		digitalWrite(_ledPin, _ledON);
	}
	filesystem.setup(true);
	if (_webServer != nullptr) {
		_webServer->addHttpHandler("/reconnectWiFi", [&](AsyncWebServerRequest* request) {
			AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "reconnect WiFi command sent");
			request->send(response);
			_wifi->reconnect();
		});
		_webServer->addHttpHandler("/reconnectMqtt", [&](AsyncWebServerRequest* request) {
			AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "reconnect mqtt command sent");
			request->send(response);
			_mqtt->reconnect();
		});
		_webServer->addHttpHandler("/syncTime", [&](AsyncWebServerRequest* request) {
			AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "NTP sync request sent");
			request->send(response);
			BasicTime::requestNtpTime();
		});
	}
	if (_mqtt != nullptr) {
		_mqtt->onConnect([&](bool sessionPresent) {
			_publishStats();
		});
		_mqtt->commands([&](BasicMqtt::Command mqttCommand) {
			if (mqttCommand[0] == "wifi") {
				if (mqttCommand.size() > 1) {
					if (mqttCommand[1] == "reconnect") {
						Serial.println("reconnecting wifi");
						_wifi->reconnect();
						return true;
					}
				}
			}
			return false;
		});
	}
	if (_wifi != nullptr) {
		_wifi->onGotIP([&](GOT_IP_HANDLER_ARGS) {
			if (_ota != nullptr) { _ota->begin(); }
			if (_mqtt != nullptr) { _mqtt->connect(); }
			if (_webServer != nullptr) { _webServer->begin(); }
		});
		_wifi->onDisconnected([&](DISCONNECTED_HANDLER_ARGS) {
			if (_mqtt != nullptr) { _mqtt->disconnect(); }
		});
	}
	if (_ota != nullptr) { _ota->setup(); }
	if (_NTPclient != nullptr) { _NTPclient->setup(); }
}
void EspBasic::_loop() {
	if (_NTPclient != nullptr) { _NTPclient->handle(); }
	if (_ota != nullptr) { _ota->handle(); }
	_loopTime();
}

void EspBasic::_publishStats() {
	if (_mqtt != nullptr) {
		_mqtt->publish((_mqtt->topicPrefix + "/rssi").c_str(), wifiRssi());
		_mqtt->publish((_mqtt->topicPrefix + "/cpu_freq").c_str(), cpuFrequency());
#ifdef ARDUINO_ARCH_ESP32
		_mqtt->publish((_mqtt->topicPrefix + "/temperature").c_str(), internalTemperature());
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
		doc["free"] = heapFree();
#ifdef ARDUINO_ARCH_ESP32
		doc["minFree"] = heapMinFree();
#elif defined(ARDUINO_ARCH_ESP8266)
		doc["stackFree"] = stackFree();
#endif
		doc["maxAlloc"] = heapMaxAlloc();
		serializeJson(doc, heapStats);
		_mqtt->publish((_mqtt->topicPrefix + "/heap").c_str(), heapStats);
	}
}
