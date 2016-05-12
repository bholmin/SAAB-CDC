#include "RN52handler.h"

void RN52handler::update() {
    if ((!eventIndicatorPinStateHandled) && (millis() - lastEventIndicatorPinStateChange) > 100) {
        if (currentEventIndicatorPinState != defaultEventIndicatorPinState) {
            unsigned long lastEventIndicatorPinStateChange = millis();
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