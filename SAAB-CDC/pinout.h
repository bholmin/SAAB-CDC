/* 
----------------------------------------------------------------------------------
 CAN.cpp
 CONTROLLER AREA NETWORK (CAN 2.0A STANDARD ID)
 CAN BUS library for Wiring/Arduino - Version 1.1
 ADAPTED FROM http://www.kreatives-chaos.com
 By IGOR REAL (03 - 03 - 2011)	
----------------------------------------------------------------------------------*/
#ifndef	PINOUT_H
#define	PINOUT_H

#ifdef __AVR_ATmega1280__
//---------------------------------------------------
#define	P_MOSI			B,2
#define	P_MISO			B,3
#define	P_SCK			B,1

#define	MCP2515_CS		B,0
//Mega pin 49
#define	MCP2515_INT		L,0
//Mega pin 19
//#define	MCP2515_INT		D,2
//---------------------------------------------------
#else
#ifdef __AVR_ATmega2560__
//---------------------------------------------------
#define	P_MOSI			B,2
#define	P_MISO			B,3
#define	P_SCK			B,1

#define	MCP2515_CS		B,0
#define	MCP2515_INT		D,2
//---------------------------------------------------
#else
//---------------------------------------------------
#define	P_MOSI			B,3
#define	P_MISO			B,4
#define	P_SCK			B,5

#define	MCP2515_CS		B,2
#define	MCP2515_INT		D,2
//---------------------------------------------------
#endif
#endif
#endif	// PINOUT_H
