#include "RN52driver.h"
#include "RN52impl.h"
#include "Timer.h"

void RN52impl::toUART(const char* c, int len){
    for(int i=0;i<len;i++)
        softSerial.write(c[i]);
};

void RN52impl::fromSPP(const char* c, int len){
    // bytes received from phone via SPP
    
    // to send bytes back to the phone call rn52.toSPP
};

void RN52impl::setMode(Mode mode){
    if (mode == COMMAND) {
        digitalWrite(BT_CMD_PIN, LOW);
    } else if (mode == DATA) {
        digitalWrite(BT_CMD_PIN, HIGH);
    }
};

const char *CMD_QUERY = "Q\r";
void RN52impl::onGPIO2() {
    queueCommand(CMD_QUERY);
}

void RN52impl::onProfileChange(BtProfile profile, bool connected) {
    switch(profile) {
        case A2DP:bt_a2dp = connected;
            if (connected && playing)bt_play();
            break;
        case SPP:bt_spp = connected;break;
        case IAP:bt_iap = connected;break;
        case HFP:bt_hfp = connected;break;
    }
}

void RN52impl::initializeAtmelPins() {
    pinMode(BT_CMD_PIN, OUTPUT);
    pinMode(BT_FACT_RST_PIN,INPUT); // Some REALLY crazy stuff is going on if this pin is set as output and pulled low. Leave it alone! Trust me...
    pinMode(BT_PWREN_PIN,OUTPUT);
    digitalWrite(BT_CMD_PIN,HIGH);
    digitalWrite(BT_PWREN_PIN,HIGH);
}

void bt_play() {
    rn52.sendAVCRP(RN52::RN52driver::PLAY);
}

void bt_pause() {
    rn52.sendAVCRP(RN52::RN52driver::PAUSE);
}

void bt_prev() {
    rn52.sendAVCRP(RN52::RN52driver::PREV);
}

void bt_next() {
    rn52.sendAVCRP(RN52::RN52driver::NEXT);
}

void bt_visible() {
    rn52.visible(true);
}

void bt_invisible() {
    rn52.visible(false);
}

void bt_reconnect() {
    rn52.reconnectLast();
}

void bt_disconnect() {
    rn52.disconnect();
}