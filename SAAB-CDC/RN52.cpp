#include "Arduino.h"
#include "RN52.h"
#include "RN52driver.h"
#include "SoftwareSerial.h"
#include "Timer.h"



/**
 * Atmel 328 pin definitions:
 */

const int BT_STATUS_PIN = 3;    // RN52 GPIO2 pin for reading current status of the module
const int BT_CMD_PIN = 4;       // RN52 GPIO9 pin for enabling command mode
const int BT_FACT_RST_PIN = A0; // RN52 factory reset pin GPIO4
const int BT_PWREN_PIN = 9;     // RN52 Power enable pin
const int UART_TX_PIN = 5;      // UART Tx
const int UART_RX_PIN = 6;      // UART Rx
extern Timer time;              // Timer instance for timed actions

SoftwareSerial softSerial =  SoftwareSerial(UART_RX_PIN, UART_TX_PIN);

void RN52Class::initializeAtmelPins() {
    pinMode(BT_CMD_PIN, OUTPUT);
    pinMode(BT_FACT_RST_PIN,INPUT); // Some REALLY crazy stuff is going on if this pin is set as output and pulled low. Leave it alone! Trust me...
    pinMode(BT_PWREN_PIN,OUTPUT);
    digitalWrite(BT_CMD_PIN,HIGH);
    digitalWrite(BT_PWREN_PIN,HIGH);
}

void RN52Class::fromUART() {
    while (softSerial.available()) {
        char c = softSerial.read();
    }
}