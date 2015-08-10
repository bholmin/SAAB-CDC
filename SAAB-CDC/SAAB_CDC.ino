//
// SAAB-CDC
//
// A CD changer emulator for older SAAB cars
//
// Code:       Seth Evans and Emil Malmberg
// CAN code:   Igor Real
// 
//

#include "CDC.h";

CDCClass CDC; //TODO: Check for a better way to do this...


// Define variables and constants

// Add setup code
void setup() {
    Serial.begin(9600);
    Serial.println("SAAB CDC Test Code - August 2015");
    CDC.initialize_atmel_pins();
    CDC.open_can_bus();
}

// Add loop code
void loop() {
    CDC.handle_cdc_status();
    //CDC.test_bt();
}