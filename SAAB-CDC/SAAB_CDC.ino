/*
 SAAB-CDC v2.0

 A CD changer emulator for older SAAB cars with RN52 bluetooth module.
 
 Credits:

 Hardware design: Seth Evans (http://bluesaab.blogspot.com)
 Initial code: Seth Evans and Emil Malmberg
 CAN code: Igor Real (http://secuduino.blogspot.com)
 Information on SAAB I-Bus: Tomi Liljemark (http://pikkupossu.1g.fi/tomi/projects/i-bus/i-bus.html)
 Additions/bug fixes: Karlis Veilands and Girts Linde

 "BEERWARE"
 As long as you retain this notice you can do whatever you want with this code.
 If we meet some day, and you think this stuff is worth it, you can buy us a beer in return.
*/


#include "Arduino.h"
#include "CDC.h"
#include "RN52.h"
#define DEBUGMODE 0 // 1 = Output debug to serial port; 0 = No output

CDCClass CDC;
RN52Class RN52;

// Setup
void setup() {
    RN52.initialize_atmel_pins();
    RN52.uart_begin();
    CDC.open_can_bus();
    Serial.begin(115200);
    Serial.println("SAAB CDC v2.0 - October 2015");
}

// Main loop
void loop() {
    CDC.handle_cdc_status();
    RN52.update();
    #if (DEBUGMODE==1) // Enabling serial input to program RN52.
        if (Serial.available() > 0) {
            char in_char[2];
            in_char[0] = Serial.read();
            in_char[1] = 0;
            RN52.write(in_char);
        }
        if (RN52.read()) {
            Serial.println(RN52.in_buffer);
        }
    #endif
}