//
// SAAB-CDC
//
// A CD changer emulator for older SAAB cars
//
// Code:       Seth Evans and Emil Malmberg
// CAN code:   Igor Real
// 
//

#include "Arduino.h";
#include "CDC.h";

CDCClass CDC; //TODO: Check for a better way to do this...


// Define variables and constants

// Add setup code
void setup() {
    Serial.begin(9600);
    Serial.println("SAAB CDC Test Code - Jun 2015");
    CDC.open_CAN_bus();
}

// Add loop code
void loop() {
    CDC.handle_CDC_status();
}
