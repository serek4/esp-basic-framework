#include "framework.hpp"

u_long loopDelay = 0;
void setup() {
	Serial.begin(115200);
	Serial.println("");
	frame.setup();
	Serial.println("Setup done!");
}
void loop() {
	if (millis() - loopDelay >= 60000) {
		loopDelay = millis();
	}
	frame.loop();
}
