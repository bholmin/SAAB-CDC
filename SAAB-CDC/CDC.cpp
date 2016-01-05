
#include "Arduino.h"
#include "CDC.h"
#include "CAN.h"
#include "RN52.h"

/**
 * Various constants used for identifying the CDC
 */

#define CDC_APL_ADR              0x12
#define CDC_SID_FUNCTION_ID      30 // Decimal

/**
 * Other useful stuff
 */

#define MODULE_NAME              "BT TEST"
#define LAST_EVENT_IN_TIMEOUT    2000 // Milliseconds
#define DISPLAY_NAME_TIMEOUT     5000 // Milliseconds
#define NODE_STATUS_TX_MSG_SIZE  4  // Defines how many messages do we need to reply with to '6A1'
#define DEBUGMODE                0 // 1 = Output debug to serial port; 0 = No output

/**
 * TX frames:
 */

#define GENERAL_STATUS_CDC       0x3C8
#define DISPLAY_RESOURCE_REQ     0x348 // 'Stolen' from the IHU since the CDC doesn't send this message
#define WRITE_TEXT_ON_DISPLAY    0x328 // 'Stolen' from the IHU since the CDC doesn't send this message
#define NODE_STATUS_TX           0x6A2
#define SOUND_REQUEST            0x430

/**
 * RX frames:
 */

#define IHU_BUTTONS              0x3C0
#define DISPLAY_RESOURCE_GRANT   0x368
#define NODE_STATUS_RX           0x6A1
#define STEERING_WHEEL_BUTTONS   0x290

/**
 * Variables:
 */

unsigned long cdc_status_last_send_time = 0; // Timer used to ensure we send the CDC status frame in a timely manner.
unsigned long display_request_last_send_time = 0; // Timer used to ensure we send the display request frame in a timely manner.
unsigned long write_text_on_display_last_send_time = 0; // Timer used to ensure we send the write text on display frame in a timely manner.
unsigned long stop_displaying_name_at = 0; // Time at which we should stop displaying our name in the SID.
unsigned long last_icoming_event_time = 0;
boolean cdc_active = false; // True while our module, the simulated CDC, is active.
boolean display_request_granted = true; // True while we are granted the 2nd row of the SID.
boolean display_wanted = false; // True while we actually want the display.
boolean cdc_status_resend_needed = false; // True if something has triggered the need to send the CDC status frame as an event.
boolean cdc_status_resend_due_to_cdc_command = false; // True if the need for sending the CDC status frame was triggered by a CDC command.
int incoming_event_counter = 0;
int ninefive_poweron_cmd[NODE_STATUS_TX_MSG_SIZE][9] = {
    {0x32,0x00,0x00,0x03,0x01,0x02,0x00,0x00,-1},
    {0x42,0x00,0x00,0x22,0x00,0x00,0x00,0x00,-1},
    {0x52,0x00,0x00,0x22,0x00,0x00,0x00,0x00,-1},
    {0x62,0x00,0x00,0x22,0x00,0x00,0x00,0x00,-1}
};
int ninefive_active_cmd[NODE_STATUS_TX_MSG_SIZE] [9] = {
    {0x32,0x00,0x00,0x16,0x01,0x02,0x00,0x00,-1},
    {0x42,0x00,0x00,0x36,0x00,0x00,0x00,0x00,-1},
    {0x52,0x00,0x00,0x36,0x00,0x00,0x00,0x00,-1},
    {0x62,0x00,0x00,0x36,0x00,0x00,0x00,0x00,-1},
};
int ninefive_powerdown_cmd[NODE_STATUS_TX_MSG_SIZE] [9] = {
    {0x32,0x00,0x00,0x19,0x01,0x00,0x00,0x00,-1},
    {0x42,0x00,0x00,0x38,0x01,0x00,0x00,0x00,-1},
    {0x52,0x00,0x00,0x38,0x01,0x00,0x00,0x00,-1},
    {0x62,0x00,0x00,0x38,0x01,0x00,0x00,0x00,-1}
};
int beep_cmd[] = {0x80,0x04,0x00,0x00,0x00,0x00,0x00,0x00,-1};
int button_release_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x00,0xFB,-1};
// int cdc_status_cmd[] = {0xE0,0xFF,0x3F,0x41,0x05,0x10,0x15,0xD0,-1}; // This results in "CD PLAY 5" being shown on SID.
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
    CAN.begin(47);  // SAAB I-Bus is 47.619kbps
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
                switch (CAN_RxMsg.data[3] & 0x0F){
                    case (0x3):
                        for (int i = 0; i < NODE_STATUS_TX_MSG_SIZE; i++) {
                            send_can_frame(NODE_STATUS_TX, ninefive_poweron_cmd[i]);
                        }
                        break;
                    case (0x2):
                        for (int i = 0; i < NODE_STATUS_TX_MSG_SIZE; i++) {
                        send_can_frame(NODE_STATUS_TX, ninefive_active_cmd[i]);
                        }
                        break;
                    case (0x8):
                        for (int i = 0; i < NODE_STATUS_TX_MSG_SIZE; i++) {
                        send_can_frame(NODE_STATUS_TX, ninefive_powerdown_cmd[i]);
                        }
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
            send_can_frame(SOUND_REQUEST, beep_cmd);
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
    
    // If the CDC status frame needs to be sent as an event, do so now
    // (note though, that we may not send the frame more often than once every 50 ms)
    
    if (cdc_status_resend_needed && (millis() - cdc_status_last_send_time > 50)) {
        send_cdc_status(true, cdc_status_resend_due_to_cdc_command);
    }
    
    // The CDC status frame must be sent with a 1000 ms periodicity.
    
    if (millis() - cdc_status_last_send_time > 850) {
        send_cdc_status(false, false);
        
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
 * Serial.write of an incoming message
 */

void CDCClass::send_serial_message(int *msg) {
    while (*msg != -1) {
        Serial.write(byte(*msg));
        msg++;
    }
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