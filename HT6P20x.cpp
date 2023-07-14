#include "HT6P20x.h"
#include <Arduino.h>

#define USING_ESP32 //COMMENT THIS IF NOT USING ESP32!!!
#define EXT_INT_GPIO_NUM 27 //ETERNAL INTERRUPT INPUT, MODIFY THIS ACCORDINGLY WITH YOUR HARDWARE...
#define REQ_CONSEC_MSGS 3 //QUANTITY OF CONSECUTIVE MESSAGES REQUIRED TO VALIDATE A REMOTE COMMAND
/**
 * these values may change depending on the HT6P20 oscillantion frequency set by the external resistor..
 * For a 2.1kHz oscillating HT6P20x I got approximately the following values (in tens of microseconds)....
*/
#define PHYS_BIT_PER_TYP 51
#define PILOT_PERIOD 1100
#define TOLERANCE 30


enum RF_State {
	WaitingPilot,
	WaitingStart,
	ReadingMsg
};

volatile RF_State rf_state = WaitingPilot;

volatile uint8_t recvKey = 0; 
volatile uint32_t recvCode = 0;
volatile bool success = false;

volatile uint32_t currMsg = 0;
volatile uint32_t lastMsg = 0;
volatile uint8_t recvMsgs = 0; //Message counter


volatile uint16_t tensOfMicros = 0;

hw_timer_t* timer = nullptr;

void IRAM_ATTR TensOfMicros_ISR() {
  tensOfMicros++;
}

static inline uint16_t abs_diff(int A, int B) {
  return ((int)(A-B) >= 0) ? (A-B) : (B-A);
}

void IRAM_ATTR RF_RX_ISR() {
	static uint8_t recvDurations = 0;
	static uint8_t recvDur[2] = {0};
	static uint8_t totalRecvDur = 0;
	static uint16_t lastTime = 0;

	uint16_t duration = tensOfMicros - lastTime;
	lastTime = tensOfMicros;

	switch(rf_state) {
		case WaitingPilot:
		if(duration >= PILOT_PERIOD) {
			rf_state = WaitingStart;
		}
		break;
		
		case WaitingStart:
		if(abs_diff(duration,PHYS_BIT_PER_TYP) < TOLERANCE) { //start bit received...
			rf_state = ReadingMsg;
		} else {
			rf_state = WaitingPilot;
		}
		break;
		
		case ReadingMsg:
		if(abs_diff(duration,PHYS_BIT_PER_TYP*2) < TOLERANCE) { //valid bit
			recvDur[recvDurations++] = duration;
			totalRecvDur++;			
			} else if (abs_diff(duration,PHYS_BIT_PER_TYP) < TOLERANCE) { //valid bit
			recvDur[recvDurations++] = duration;
			totalRecvDur++;
			} else { //garbage, reset everything...
			recvDurations = 0;
			recvCode = 0;
			recvKey = 0;
			currMsg = 0;
			lastMsg = 0;
			recvMsgs = 0;
			totalRecvDur = 0;
			rf_state = WaitingPilot;
			break;
		}
		if(recvDurations == 2) {
			recvDurations = 0;
			currMsg <<= 1;
			if(recvDur[0] > recvDur[1]) { //one
				currMsg |= 1;
			} //else is a zero, i.e. just do the right shift but no "bit-wise OR zero"
		}
		
		if(totalRecvDur == 56) {
			recvDurations = 0;
			totalRecvDur = 0;
			uint8_t antiCode = (currMsg & 0x0000000f);
			rf_state = WaitingPilot;
			if(antiCode != 0b0101) { //invalid message....
				recvCode = 0;
				recvKey = 0;
				lastMsg = 0;
				currMsg = 0;
				recvMsgs = 0;
				break;
			}
			
			if(lastMsg == 0) {
				lastMsg = currMsg;
				currMsg = 0;
				recvMsgs++;
				break;
			} else {
				if(currMsg == lastMsg) recvMsgs++;
				else { //consecutive messages differ...
					recvCode = 0;
					recvKey = 0;
					lastMsg = 0;
					currMsg = 0;
					recvMsgs = 0;
					break;
				}
				
				if(recvMsgs >= REQ_CONSEC_MSGS){
					lastMsg = 0;
					recvMsgs = 0;
					recvCode = (currMsg & 0xffffff00)>>8;
					recvKey = (currMsg & 0x000000f0)>>4;
					success = true; //this is just used to flag the main polling to print the received code+key...
				}
				currMsg = 0;
			}
		}
		break;
		
		default:
		break;	
	}
}

void RF_RX_Init() {
	timer = timerBegin(0, 80, true); //set the timer clock
	timerAlarmWrite(timer, 10, true); //set timer to auto-reload every 10 microseconds
	timerAttachInterrupt(timer, &TensOfMicros_ISR, true); //register the timer ISR
	timerAlarmEnable(timer);

	#ifndef USING_ESP32
		#warning Please use the interrupt resolution function to properly map pin to interrupt
	#endif

	attachInterrupt(EXT_INT_GPIO_NUM, RF_RX_ISR, CHANGE); //external interrupt triggered by both rising and falling edges
}
