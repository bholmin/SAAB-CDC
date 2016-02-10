
#include "Arduino.h"
#include "CDC.h"
#include "CAN.h"
#include "RN52.h"
#include "Timer.h"

/**
 * Variables:
 */

extern Timer time;
void send_cdc_node_status(void*);
void send_cdc_active_status(void*);
void send_cdc_powerdown_status(void*);
void *current_cdc_cmd = NULL;
unsigned long cdc_status_last_send_time = 0;            // Timer used to ensure we send the CDC status frame in a timely manner.
unsigned long display_request_last_send_time = 0;       // Timer used to ensure we send the display request frame in a timely manner.
unsigned long write_text_on_display_last_send_time = 0; // Timer used to ensure we send the write text on display frame in a timely manner.
unsigned long stop_displaying_name_at = 0;              // Time at which we should stop displaying our name in the SID.
unsigned long last_icoming_event_time = 0;              // Timer used for determening if we should treat current event as, for example, a long press of a button.
boolean cdc_active = false;                             // True while our module, the simulated CDC, is active.
boolean display_request_granted = true;                 // True while we are granted the 2nd row of the SID.
boolean display_wanted = false;                         // True while we actually want the display.
boolean cdc_status_resend_needed = false;               // True if something has triggered the need to send the CDC status frame as an event.
boolean cdc_status_resend_due_to_cdc_command = false;   // True if the need for sending the CDC status frame was triggered by a CDC command.
int incoming_event_counter = 0;                         // Counter for incoming events to determine when we will treat the event, for example, as a long press of a button.
int current_timer_event = -1;
int cdc_poweron_cmd[NODE_STATUS_TX_MSG_SIZE][9] = {
    {0x32,0x00,0x00,0x03,0x01,0x02,0x00,0x00,-1},
    {0x42,0x00,0x00,0x22,0x00,0x00,0x00,0x00,-1},
    {0x52,0x00,0x00,0x22,0x00,0x00,0x00,0x00,-1},
    {0x62,0x00,0x00,0x22,0x00,0x00,0x00,0x00,-1}
};
int cdc_active_cmd[NODE_STATUS_TX_MSG_SIZE] [9] = {
    {0x32,0x00,0x00,0x16,0x01,0x02,0x00,0x00,-1},
    {0x42,0x00,0x00,0x36,0x00,0x00,0x00,0x00,-1},
    {0x52,0x00,0x00,0x36,0x00,0x00,0x00,0x00,-1},
    {0x62,0x00,0x00,0x36,0x00,0x00,0x00,0x00,-1},
};
int cdc_powerdown_cmd[NODE_STATUS_TX_MSG_SIZE] [9] = {
    {0x32,0x00,0x00,0x19,0x01,0x00,0x00,0x00,-1},
    {0x42,0x00,0x00,0x38,0x01,0x00,0x00,0x00,-1},
    {0x52,0x00,0x00,0x38,0x01,0x00,0x00,0x00,-1},
    {0x62,0x00,0x00,0x38,0x01,0x00,0x00,0x00,-1}
};
int sound_cmd[] = {0x80,SOUND_ALERT,0x00,0x00,0x00,0x00,0x00,0x00,-1};
int cdc_status_cmd[] = {0xE0,0xFF,0x3F,0x41,0xFF,0xFF,0xFF,0xD0,-1};
int display_request_cmd[] = {CDC_APL_ADR,0x02,0x02,CDC_SID_FUNCTION_ID,0x00,0x00,0x00,0x00,-1};

/******************************************************************************
 * PUBLIC METHODS
 ******************************************************************************/

/**
 * DEBUG: Prints the CAN TX frame to serial output
 */

void CDCClass::print_can_tx_frame() {
    Serial.print(CAN_TxMsg.id,HEX);
    Serial.print(" -> ");
    for (int i = 0; i < 8; i++) {
        Serial.print(CAN_TxMsg.data[i],HEX);
        Serial.print(" ");
    }
    Serial.println();
}

/**
 * DEBUG: Prints the CAN RX frame to serial output
 */

void CDCClass::print_can_rx_frame() {
        Serial.print(CAN_RxMsg.id,HEX);
        Serial.print(" -> ");
        for (int i = 0; i < 8; i++) {
            Serial.print(CAN_RxMsg.data[i],HEX);
            Serial.print(" ");
        }
        Serial.println();
}

/**
 * Opens CAN bus for communication
 */

void CDCClass::open_can_bus() {
    #if (DEBUGMODE==1)
        Serial.println("DEBUG: Initializing CAN bus @ 47.619kbps");
    #endif
    CAN.begin(47);                // SAAB I-Bus is 47.619kbps
    CAN_TxMsg.header.rtr = 0;     // This value never changes
    CAN_TxMsg.header.length = 8;  // This value never changes
}

/**
 * Handles an incoming (RX) frame
 */

void CDCClass::handle_rx_frame() {
    if (CAN.CheckNew()) {
        CAN_TxMsg.data[0]++;
        CAN.ReadFromDevice(&CAN_RxMsg);
        switch (CAN_RxMsg.id) {
            case NODE_STATUS_RX:
                // Here be dragons... This part of the code is responsible for causing lots of head ache.
                // We look at the bottom half of 3rd byte of '6A1' frame to determine what 'current_cdc_command' should be.
                switch (CAN_RxMsg.data[3] & 0x0F){
                    case (0x3):
                        current_cdc_cmd = cdc_poweron_cmd;
                        send_cdc_node_status(NULL);
                        break;
                    case (0x2):
                        current_cdc_cmd = cdc_active_cmd;
                        send_cdc_node_status(NULL);
                        break;
                    case (0x8):
                        current_cdc_cmd = cdc_powerdown_cmd;
                        send_cdc_node_status(NULL);
                        break;
                }
                break;
            case IHU_BUTTONS:
                handle_ihu_buttons();
                break;
            case STEERING_WHEEL_BUTTONS:
                handle_steering_wheel_buttons();
                break;
            case DISPLAY_RESOURCE_GRANT:
                if ((CAN_RxMsg.data[1] == 0x02) && (CAN_RxMsg.data[3] == CDC_SID_FUNCTION_ID)) {
                    #if (DEBUGMODE==1)
                        Serial.println("DEBUG: We have been granted the right to write text to the second row in the SID.");
                    #endif
                    display_request_granted = true;
                    write_text_on_display(MODULE_NAME);
                }
                else if (CAN_RxMsg.data[1] == 0x02) {
                    #if (DEBUGMODE==1)
                        Serial.println("DEBUG: Someone else has been granted the second row, we need to back down");
                    #endif
                    display_request_granted = false;
                }
                else if (CAN_RxMsg.data[1] == 0x00) {
                    #if (DEBUGMODE==1)
                        Serial.println("DEBUG: Someone else has been granted the entire display, we need to back down");
                    #endif
                    display_request_granted = false;
                }
                else {
                    #if (DEBUGMODE==1)
                        Serial.println("DEBUG: Someone else has been granted the first row; if we had the grant to the 2nd row, we still have it.");
                    #endif
                }
                break;
        }
    }
}

/**
 * Handles the IHU_BUTTONS frame that the IHU sends us when it wants to control some feature of the CDC
 */

void CDCClass::handle_ihu_buttons() {
    #if (DEBUGMODE==1)
        print_can_rx_frame();
    #endif
    
    boolean event = (CAN_RxMsg.data[0] == 0x80);
    if (!event) {
        // FIXME: can we really ignore the message if it wasn't sent on event?
        return;
    }
    switch (CAN_RxMsg.data[1]) {
        case 0x24: // CDC = ON (CD/RDM button has been pressed twice)
            cdc_active = true;
            send_can_frame(SOUND_REQUEST, sound_cmd);
            RN52.start_connecting();
            break;
        case 0x14: // CDC = OFF (Back to Radio or Tape mode)
            cdc_active = false;
            display_wanted = false;
            RN52.start_disconnecting();
            break;
            }
    if (cdc_active) {
        #if (DEBUGMODE==1)
            print_can_rx_frame();
        #endif
        switch (CAN_RxMsg.data[1]) {
            case 0x59: // NXT
                RN52.write(PLAYPAUSE);
                break;
            case 0x45: // SEEK+ button long press on 9-3 IHU
                RN52.write(CONNECT);
                break;
            case 0x46: // SEEK- button long press on 9-3 IHU
                RN52.write(DISCONNECT);
                break;
            case 0x84: // SEEK button long press on IHU
                RN52.write(CONNECT);
                break;
            case 0X88: // > 2 sec long press of SEEK button on IHU
                RN52.write(DISCONNECT);
                break;
            case 0x76: // Random ON/OFF (Long press of CD/RDM button)
                RN52.write(VOLUP);
                break;
            case 0xB1: // Pause ON
                // N/A for now
                break;
            case 0xB0: // Pause OFF
                // N/A for now
                break;
            case 0x35: // Track +
                RN52.write(NEXTTRACK);
                break;
            case 0x36: // Track -
                RN52.write(PREVTRACK);
                break;
            default:
                break;
        }
    }
}

/**
 * Handles the STEERING_WHEEL_BUTTONS frame
 * TODO connect the SID button events to actions
 */

void CDCClass::handle_steering_wheel_buttons() {
    #if (DEBUGMODE==1)
        print_can_rx_frame();
    #endif
    if (!cdc_active) {
        return;
    }
    boolean event = (CAN_RxMsg.data[0] == 0x80);
    if (!event) {
        /*
        // Possible long press of a button has occured. We need to handle this.
        if (millis() - last_icoming_event_time > LAST_EVENT_IN_TIMEOUT) {
            incoming_event_counter = 0;
        }
        incoming_event_counter++;
        last_icoming_event_time = millis();
        switch (CAN_RxMsg.data[4]) {
            case 0x04: // Long press of NXT button on wheel
                if (incoming_event_counter == 5) {
                    RN52.write(ASSISTANT);
                }
                break;
            default:
                break;
        }
         */
        return;
    }
    switch (CAN_RxMsg.data[2]) {
        case 0x04: // NXT button on wheel
            #if (DEBUGMODE==1)
                Serial.println("DEBUG: 'NXT' button on wheel pressed.");
            #endif
            //RN52.write(PLAYPAUSE);
            break;
        case 0x10: // Seek+ button on wheel
            #if (DEBUGMODE==1)
                Serial.println("DEBUG: 'Seek+' button on wheel pressed.");
            #endif
            //RN52.write(NEXTTRACK);
            break;
        case 0x08: // Seek- button on wheel
            #if (DEBUGMODE==1)
                Serial.println("DEBUG: 'Seek-' button on wheel pressed.");
            #endif
            //RN52.write(PREVTRACK);
            break;
        default:
            #if (DEBUGMODE==1)
                Serial.print(CAN_RxMsg.data[2],HEX);
                Serial.println("DEBUG: Unknown button message");
            #endif
            break;
    }
}

/**
 * Handles CDC status and sends it to IHU as necessary
 */

void CDCClass::handle_cdc_status() {
    
    handle_rx_frame();
    
    if (cdc_status_resend_needed && (millis() - cdc_status_last_send_time > CDC_STATUS_RE_TX_TIME)) {
        send_cdc_status(true, cdc_status_resend_due_to_cdc_command);
    }

}

void CDCClass::send_cdc_status(boolean event, boolean remote) {
    send_can_frame(GENERAL_STATUS_CDC, cdc_status_cmd);
    
    // Record the time of sending and reset status variables
    cdc_status_last_send_time = millis();
    cdc_status_resend_needed = false;
    cdc_status_resend_due_to_cdc_command = false;
    
}

/**
 * Sends a request for using the SID, row 2. We may NOT start writing until we've received a grant frame with the correct function ID!
 */

void CDCClass::send_display_request() {
    send_can_frame(DISPLAY_RESOURCE_REQ, display_request_cmd);
    display_request_last_send_time = millis();
}

/**
 * Formats and puts a frame on CAN bus
 */

void CDCClass::send_can_frame(int message_id, int *msg) {
    CAN_TxMsg.id = message_id;
    int i = 0;
    while (msg[i] != -1) {
        CAN_TxMsg.data[i] = msg[i];
        i++;
    }
    CAN.send(&CAN_TxMsg);
}

/**
 * Sends a reply of four messages to '6A1' requests
 */

void send_cdc_node_status(void *p) {
    int i = (int)p;
    
    if (current_timer_event != -1) {
        time.stop(current_timer_event);
    }
    CDC.send_can_frame(NODE_STATUS_TX, ((int(*)[9])current_cdc_cmd)[i]);
    if (i < 3) {
        current_timer_event = time.after(NODE_STATUS_TX_TIME,send_cdc_node_status,(void*)(i + 1));
    }    
    else current_timer_event = -1;
}

/**
 * Sends CDC status very CDC_STATUS_TX_TIME interval
 */

void send_cdc_status_on_time(void*) {
    CDC.send_cdc_status(false, false);
}

/**
 * Writes the provided text on the SID. This function assumes that we have been granted write access. Do not call it if we haven't!
 * NOTE the character set used by the SID is slightly nonstandard. "Normal" characters should work fine.
 */

void CDCClass::write_text_on_display(char text[]) {
    if (!text) {
        return;
    }
    
    // Copy the provided string and make sure we have a new array of the correct length:
    
    char txt[15];
    int i, n;
    n = strlen(text);
    n = n > 12 ? 12 : n;
    for (i = 0; i < n; i++) {
        txt[i] = text[i];
    }
    for (i = n + 1; i < 16; i++) {
        txt[i] = 0;
    }
    
    CAN_TxMsg.id = WRITE_TEXT_ON_DISPLAY;
    
    CAN_TxMsg.data[0] = 0x42; // TODO: check if this is really correct? According to the spec, the 4 shouldn't be there? It's just a normal transport layer sequence numbering?
    CAN_TxMsg.data[1] = 0x96; // Address of the SID
    CAN_TxMsg.data[2] = 0x02; // Sent on basetime, writing to row 2
    CAN_TxMsg.data[3] = txt[0];
    CAN_TxMsg.data[4] = txt[1];
    CAN_TxMsg.data[5] = txt[2];
    CAN_TxMsg.data[6] = txt[3];
    CAN_TxMsg.data[7] = txt[4];
    CAN.send(&CAN_TxMsg);
    
    CAN_TxMsg.data[0] = 0x01; // message 1
    CAN_TxMsg.data[3] = txt[5];
    CAN_TxMsg.data[4] = txt[6];
    CAN_TxMsg.data[5] = txt[7];
    CAN_TxMsg.data[6] = txt[8];
    CAN_TxMsg.data[7] = txt[9];
    CAN.send(&CAN_TxMsg);
    
    CAN_TxMsg.data[0] = 0x00; // message 0
    CAN_TxMsg.data[3] = txt[10];
    CAN_TxMsg.data[4] = txt[11];
    CAN_TxMsg.data[5] = txt[12];
    CAN_TxMsg.data[6] = txt[13];
    CAN_TxMsg.data[7] = txt[14];
    CAN.send(&CAN_TxMsg);
    
    write_text_on_display_last_send_time = millis();
}
