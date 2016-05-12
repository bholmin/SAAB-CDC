#ifndef RN52HANDLER_H
#define RN52HANDLER_H

#include "Arduino.h"
#include "RN52impl.h"

class RN52handler {
    RN52impl driver;
    bool eventIndicatorPinStateHandled;
    int defaultEventIndicatorPinState = 1;
    int currentEventIndicatorPinState = digitalRead(BT_EVENT_INDICATOR_PIN);
    unsigned long lastEventIndicatorPinStateChange = 0;
    
public:
    void update();
    void bt_play();
    void bt_pause();
    void bt_prev();
    void bt_next();
    void bt_visible();
    void bt_invisible();
    void bt_reconnect();
    void bt_disconnect();
};

#endif
