/*
 * Virtual C++ Class for RovingNetworks RN-52 Bluetooth modules
 * Copyright (C) 2013  Tim Otto
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Created by: Tim Otto
 * Created on: Jun 21, 2013
 * Modified by: Karlis Veilands
 * Modified on: May 17, 2016
 */

#include "RN52handler.h"

RN52handler BT;

/**
 * Checks for state of event indicator pin (GPIO2). Calls out onGPIO2() from RN52impl that will querry the RN52 module for its status.
 */

void RN52handler::update() {
    driver.readFromUART();
    if (digitalRead(BT_EVENT_INDICATOR_PIN) == 0) {
    if ((millis() - lastEventIndicatorPinStateChange) > 100) {
        Serial.println("DEBUG: RN52 GPIO2 status has changed. Handling...");
        lastEventIndicatorPinStateChange = millis();
        driver.onGPIO2();
        }
    }
}

void RN52handler::bt_play() {
    driver.sendAVCRP(RN52::RN52driver::PLAYPAUSE);
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

void RN52handler::bt_vassistant() {
    driver.sendAVCRP(RN52::RN52driver::VASSISTANT);
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

/**
 * Debug function used only in 'bench' testing. Listens to input on serial console and calls out corresponding function.
 */

void RN52handler::monitor_serial_input() {
    int incomingByte = 0;
    
    if (Serial.available() > 0) {
        incomingByte = Serial.read();
        switch (incomingByte) {
            case 'V':
                bt_visible();
                break;
            case 'I':
                bt_invisible();
                break;
            case 'C':
                bt_reconnect();
                break;
            case 'D':
                bt_disconnect();
                break;
            case 'P':
                bt_play();
                break;
            case 'N':
                bt_next();
                break;
            case 'R':
                bt_prev();
                break;
            case 'A':
                bt_vassistant();
                break;
            default:
                break;
        }
    }
}

void RN52handler::initialize() {
    driver.initialize();
}