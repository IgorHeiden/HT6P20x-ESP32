#include <Arduino.h>
#include "HT6P20x.h"

void setup() {
	Serial.begin(115200);
	
	RF_RX_Init();
}

void loop() {

	if(success) {
		success = false;
		Serial.printf("code: %x.\r\n",recvCode);
		Serial.printf("key: %x.\r\n",recvKey);
	}
  
}
