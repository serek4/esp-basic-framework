#pragma once

#include <Arduino.h>
#include <esp-basic-config.hpp>
#include <esp-basic-fs.hpp>
#include <esp-basic-logs.hpp>
#include <esp-basic-mqtt.hpp>
#include <esp-basic-ota.hpp>
#include <esp-basic-time.hpp>
#include <esp-basic-web-server.hpp>
#include <esp-basic-wifi.hpp>
#include <list>

// #define BASIC_FRAME_DEBUG
// printing macros
// clang-format off
#ifdef BASIC_FRAME_DEBUG
#define DEBUG_PRINTER Serial
#define BASIC_FRAME_PRINT(...){ DEBUG_PRINTER.print(__VA_ARGS__); }
#define BASIC_FRAME_PRINTLN(...){ DEBUG_PRINTER.println(__VA_ARGS__); }
#define BASIC_FRAME_PRINTF(...){ DEBUG_PRINTER.printf(__VA_ARGS__); }
#else
#define BASIC_FRAME_PRINT(...){}
#define BASIC_FRAME_PRINTLN(...){}
#define BASIC_FRAME_PRINTF(...){}
#endif
// clang-format on

#define BLINK_ONCE 1
#define ONE_MINUTE 60000
#define HEAP_FREE ESP.getFreeHeap()
#define WIFI_RSSI WiFi.RSSI()
#ifdef ARDUINO_ARCH_ESP32
#define CPU_FREQUENCY getCpuFrequencyMhz()
#define HEAP_MIN_FREE ESP.getMinFreeHeap()
#define HEAP_MAX_ALLOC ESP.getMaxAllocHeap()
#elif defined(ARDUINO_ARCH_ESP8266)
#define CPU_FREQUENCY ESP.getCpuFreqMHz()
#define STACK_FREE ESP.getFreeContStack()
#define HEAP_MAX_ALLOC ESP.getMaxFreeBlockSize()
#endif

class EspBasic {
  private:
	u_long _prevLoopTime;
	uint32_t _loopCount;
	uint32_t _avgLoopBuffer;

	uint16_t _loopTime();

  protected:
	uint8_t _ledPin;
	bool _ledON;
	bool _useLed;
	u_long _1minTimer;

	uint16_t _avgLoopTime();
	void _loop();
	// clang-format off
	int8_t wifiRssi() { return WIFI_RSSI; }
	uint32_t cpuFrequency() { return CPU_FREQUENCY; }
#ifdef ARDUINO_ARCH_ESP32
	float internalTemperature() { return temperatureRead(); }
	uint32_t heapMinFree() { return HEAP_MIN_FREE; }
#elif defined(ARDUINO_ARCH_ESP8266)
	uint32_t stackFree() { return STACK_FREE; }
#endif
	uint32_t heapFree() { return HEAP_FREE; }
	uint32_t heapMaxAlloc() { return HEAP_MAX_ALLOC; }
	// clang-format on

  public:
	EspBasic();
	EspBasic(uint8_t ledPin, bool ONstate, bool useLed = true);

	uint16_t loopTime;
	uint16_t avgLoopTime;
	uint32_t loopCount;
	uint32_t avgLoopBuffer;

	void begin();
	void blinkLed(u_long onTime, u_long offTime, uint8_t repeat = BLINK_ONCE);
	/**
	 * put this as last command in main setup()
	*/
	void setupDone();
};
