#ifndef RN52HANDLER_H
#define RN52HANDLER_H

#include "Arduino.h"
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
