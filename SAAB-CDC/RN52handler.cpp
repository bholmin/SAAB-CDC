#include "RN52handler.h"

/**
 * Checks for state of event indicator pin (GPIO2). Calls out onGPIO2() from RN52impl that will querry the RN52 module for its status.
 */

void RN52handler::update() {
    if ((!eventIndicatorPinStateHandled) && (millis() - lastEventIndicatorPinStateChange) > 100) {
        if (digitalRead(BT_EVENT_INDICATOR_PIN) == 0) {
            Serial.println("DEBUG: GPIO2 status has changed. Handling...");
            lastEventIndicatorPinStateChange = millis();
            driver.onGPIO2();
            eventIndicatorPinStateHandled = true;
        }
    }
}

void RN52handler::bt_play() {
    driver.sendAVCRP(RN52::RN52driver::PLAY);
}

void RN52handler::bt_pause() {
    driver.sendAVCRP(RN52::RN52driver::PAUSE);
}

void RN52handler::bt_prev() {
    driver.sendAVCRP(RN52::RN52driver::PREV);
}

void RN52handler::bt_next() {
    driver.sendAVCRP(RN52::RN52driver::NEXT);
}

void RN52handler::bt_visible() {
    driver.visible(true);
}

void RN52handler::bt_invisible() {
    driver.visible(false);
}

void RN52handler::bt_reconnect() {
    driver.reconnectLast();
}

void RN52handler::bt_disconnect() {
    driver.disconnect();
}

void RN52handler::monitor_serial_input() {
    int incomingByte = 0;
    
    if (Serial.available() > 0) {
        incomingByte = Serial.read();
        switch (incomingByte) {
            case 'V':
                Serial.println("RN52: \"Making module discoverable.");
                bt_visible();
                break;
            case 'I':
                Serial.println("RN52: \"Making module non-discoverable/connectable.");
                bt_invisible();
                break;
            case 'C':
                Serial.println("RN52: \"Manual connect to the last known device.");
                bt_reconnect();
                break;
            case 'D':
                Serial.println("RN52: \"Manual disconnect from the device.");
                bt_disconnect();
                break;
            case 'P':
                Serial.println("RN52: \"Sending 'Play' command.");
                bt_play();
                break;
            case 'N':
                Serial.println("RN52: \"Sending 'Next Track' command.");
                bt_next();
                break;
            case 'R':
                Serial.println("RN52: \"Sending 'Previous Track' command.");
                bt_prev();
                break;
            case 'L':
                Serial.println("RN52: \"PWREN pin HIGH.");
                digitalWrite(BT_PWREN_PIN,HIGH);
                break;
            case 'l':
                Serial.println("RN52: \"PWREN pin LOW.");
                digitalWrite(BT_PWREN_PIN,LOW);
                break;
            default:
                break;
        }
    }
}

void RN52handler::initialize() {
    driver.initialize();
}