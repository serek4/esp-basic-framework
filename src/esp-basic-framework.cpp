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
    , avgLoopBuffer(0) {
}
EspBasic::EspBasic()
    : EspBasic::EspBasic(255, LOW, false) {
}

void EspBasic::begin() {
	if (_useLed) {
		pinMode(_ledPin, OUTPUT);
		digitalWrite(_ledPin, _ledON);
	}
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

void EspBasic::_loop() {
	_loopTime();
}