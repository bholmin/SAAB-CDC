#ifndef RN52HANDLER_H
#define RN52HANDLER_H

#include "Arduino.h"
#include "RN52impl.h"

class RN52handler {
    RN52impl driver;
    
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
