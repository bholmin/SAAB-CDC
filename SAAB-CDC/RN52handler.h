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

#ifndef RN52HANDLER_H
#define RN52HANDLER_H

#include <Arduino.h>
#include "RN52impl.h"

class RN52handler {
    RN52impl driver;
    
    unsigned long lastEventIndicatorPinStateChange;
    
public:
    RN52handler() {
        lastEventIndicatorPinStateChange = 0;
    }
    void update();
    void bt_play();
    void bt_pause();
    void bt_prev();
    void bt_next();
    void bt_vassistant();
    void bt_visible();
    void bt_invisible();
    void bt_reconnect();
    void bt_disconnect();
    void monitor_serial_input();
    void initialize();
};

extern RN52handler BT;

#endif
