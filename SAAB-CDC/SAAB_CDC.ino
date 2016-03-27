/*
 SAAB-CDC v2.1

 A CD changer emulator for older SAAB cars with RN52 bluetooth module.
 
 Credits:

 Hardware design: Seth Evans (http://bluesaab.blogspot.com)
 Initial code: Seth Evans and Emil Fors
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
#include "Timer.h"

CDCClass CDC;
RN52Class RN52;
Timer time;


// Setup
void setup() {
    Serial.begin(BAUDRATE);
    Serial.println("SAAB CDC-DEV v2.1 - March 2016");
    RN52.initialize_atmel_pins();
    CDC.open_can_bus();
    time.every(CDC_STATUS_TX_TIME, &send_cdc_status_on_time,NULL);
}

// Main loop
void loop() {
    time.update();
    CDC.handle_cdc_status();
    RN52.update();
    RN52.monitor_serial_input();
}
