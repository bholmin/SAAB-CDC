/*
 SAAB-CDC v3.0

 A CD changer emulator for older SAAB cars with RN52 bluetooth module.
 
 Credits:

 Hardware design: Seth Evans (http://bluesaab.blogspot.com)
 Initial CDC code: Seth Evans and Emil Fors
 CAN code: Igor Real (http://secuduino.blogspot.com)
 Information on SAAB I-Bus: Tomi Liljemark (http://pikkupossu.1g.fi/tomi/projects/i-bus/i-bus.html)
 RN52 handing:  based on code by Tim Otto
 Additions/bug fixes: Karlis Veilands and Girts Linde

 "BEERWARE"
 As long as you retain this notice you can do whatever you want with this code.
 If we meet some day, and you think this stuff is worth it, you can buy us a beer in return.
*/


#include <Arduino.h>
#include "CDC.h"
#include "RN52handler.h"
#include "Timer.h"

CDChandler CDC;
Timer time;


// Setup
void setup() {
    Serial.begin(9600);
    Serial.println("SAAB-CDC v3.0 - June 2016");
    CDC.openCanBus();
    BT.initialize();
    time.every(CDC_STATUS_TX_TIME, &sendCdcStatusOnTime,NULL);
}

// Main loop
void loop() {
    time.update();
    CDC.handleCdcStatus();
    BT.update();
    BT.monitor_serial_input();
}
