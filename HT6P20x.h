#ifndef HT6P20X_H
#define HT6P20X_H

#include <cstdint>


/** 
 *	the variable below corresponds to the D3~D0 "data" bytes in the HT6P20x manual,this is usually
 *  mapped to some remote control key, thus the varible name...
*/
extern volatile uint8_t recvKey; 

extern volatile uint32_t recvCode; //this corresponds to the A0~A19 "address" bytes in the HT6P20x manual

extern volatile bool success;


void RF_RX_Init(void);

#endif
