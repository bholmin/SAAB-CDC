//
//  CDC.cpp
//  SAAB-CDC
//
//  Created by:  Seth Evans and Emil Malmberg
//  Refactoring: Karlis Veilands
//
//

#include "CDC.h"
#include "CAN.h"

/**
 * Various constants used for identifying the CDC
 */

#define CDC_APL_ADR              0x12
#define CDC_SID_FUNCTION_ID      30 // decimal

/**
 * Other useful stuff
 */

#define MODULE_NAME              "BLUETOOTH"
#define DISPLAY_NAME_TIMEOUT     5000

/**
 * TX frames:
 */

#define GENERAL_STATUS_CDC       0x3c8
#define DISPLAY_RESOURCE_REQ     0x348 // 'stolen' from the IHU since the CDC doesn't send this message
#define WRITE_TEXT_ON_DISPLAY    0x328 // 'stolen' from the IHU since the CDC doesn't send this message
#define NODE_STATUS_TX           0x6a2
#define SOUND_REQUEST            0x430

/**
 * RX frames:
 */

#define CDC_CONTROL              0x3c0
#define DISPLAY_RESOURCE_GRANT   0x368
#define NODE_STATUS_RX           0x6a1
#define SID_BUTTONS              0x290

/**
 * Variables:
 */

unsigned long cdc_status_last_send_time = 0; // timer used to ensure we send the CDC status frame in a timely manner
unsigned long display_request_last_send_time = 0; // timer used to ensure we send the display request frame in a timely manner
unsigned long write_text_on_display_last_send_time = 0; // timer used to ensure we send the write text on display frame in a timely manner
unsigned long stop_displaying_name_at = 0; // the time at which we should stop displaying our name in the SID
boolean cdc_active = false; // true while our module, the simulated CDC, is active
boolean display_request_granted = true; // true while we are granted the 2nd row of the SID
boolean display_wanted = false; // true while we actually WANT the display
boolean cdc_status_resend_needed = false; // true if something has triggered the need to send the CDC status frame as an event
boolean cdc_status_resend_due_to_cdc_command = false; // true if the need for sending the CDC status frame was triggered by a CDC command
int power_pin = 7;
int play_pin = 5;
int forward_pin = 6;
int previous_pin = 8;
int toggle_shuffle = 1; // TODO: switch to boolean?
int mute = 0; // TODO: switch to boolean?
int ninefive_cmd[] = {0x62,0x00,0x00,0x16,0x01,0x02,0x00,0x00};
int beep_cmd[] = {0x80,0x04,0x00,0x00,0x00,0x00,0x00,0x00};
int playipod_cmd[] = {0xFF,0x55,0x04,0x02,0x00,0x00,0x01,0xF9};
int playpauseipod_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x01,0xFA};
int stopipod_cmd[] = {0xFF,0x55,0x04,0x02,0x00,0x00,0x02,0xF8};
int next_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x08,0xF3};
int prev_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x10,0xEB};
int shuffle_cmd[] = {0xFF,0x55,0x04,0x02,0x00,0x00,0x80,0x7A};
int repeat_cmd[] = {0xFF,0x55,0x05,0x02,0x00,0x00,0x00,0x01,0xF8};
int button_release_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x00,0xFB};

/******************************************************************************
 * PUBLIC METHODS
 ******************************************************************************/

void CDCClass::open_CAN_bus()
{
    CAN.begin(47);  // Saab I-Bus is 47.619kbps
    CAN_TxMsg.header.rtr = 0;     // this value never changes
    CAN_TxMsg.header.length = 8;  // this value never changes
}

/**
 * Prints the contents of CAN bus to serial console.
 */

void CDCClass::print_bus()
{
    if (CAN_RxMsg.id==0x348) {
        //if (CAN_RxMsg.data[0]==0x80) {
        Serial.print(CAN_RxMsg.id,HEX);
        Serial.print(";");
        for (int i = 0; i < 8; i++) {
            Serial.print(CAN_RxMsg.data[i],HEX);
            Serial.print(";");
        }
        Serial.println("");
    }
}

/******************************************************************************
 * PRIVATE METHODS
 ******************************************************************************/

/**
 * Handles connection with the BC05B Bluetooth module.
 */

void CDCClass::handle_BT_connection()
{
    pinMode(power_pin, OUTPUT);
    digitalWrite(power_pin,LOW);
}

/**
 * Handles an incoming (RX) frame.
 */

void CDCClass::handle_RX_frame()
{
    switch (CAN_RxMsg.id) {
        case NODE_STATUS_RX:
            // FIXME: we should check exactly what this network node message is and what the reply should be. For now, go with Seth's original code:
            CAN_TxMsg.id = NODE_STATUS_TX;
            for (int c = 0; c < 8; c++) {
                CAN_TxMsg.data[c] = ninefive_cmd[c];
            }
            CAN.send(&CAN_TxMsg);
            break;
            
        case CDC_CONTROL:
            handle_CDC_control();
            break;
            
        case SID_BUTTONS:
            handle_SID_buttons();
            break;
            
        case DISPLAY_RESOURCE_GRANT:
            if ((CAN_RxMsg.data[1] == 0x02) && (CAN_RxMsg.data[3] == CDC_SID_FUNCTION_ID)) {
                // we have been granted the right to write text to the second row in the SID
                Serial.begin(9600);
                Serial.println("SID");
                Serial.end();
                display_request_granted = true;
            }
            else if (CAN_RxMsg.data[1] == 0x02) {
                Serial.begin(9600);
                Serial.println("Someone else has been granted the second row, we need to back down");
                Serial.end();
                display_request_granted = false;
            }
            else if (CAN_RxMsg.data[1] == 0x00) {
                Serial.begin(9600);
                Serial.println("Someone else has been granted the entire display, we need to back down");
                Serial.end();
                display_request_granted = false;
            } 
            else {
                //someone else has been granted the first row; if we had the grant to the 2nd row, we still have it
            }
            break;
    }
}

/**
 * Handles the CDC_CONTROL frame that the IHU sends us when it wants to control some feature of the CDC.
 */

void CDCClass::handle_CDC_control()
{
    boolean event = (CAN_RxMsg.data[0] == 0x80);
    
    if (!event) {
        //FIXME: can we really ignore the message if it wasn't sent on event?
        return;
    }
    
    switch (CAN_RxMsg.data[1]) {
        case 0x24: // Play
            cdc_active = true;
            
            // we want to show the module name in the SID for a little while after being activated:
            stop_displaying_name_at = millis() + DISPLAY_NAME_TIMEOUT;
            display_wanted = true;
            
            /*for (int j = 0; j < 8; j++) {
             CAN_TxMsg.id = SOUND_REQUEST;
             CAN_TxMsg.data[j] = beep_cmd[j];
             }
             CAN.send(&CAN_TxMsg);
             */
            for (int j = 0; j < 8; j++) {
                Serial.write(byte(playipod_cmd[j]));
            }
            delay(3);
            //Serial.println("Release");
            for (int i = 0; i < 7; i++) {
                Serial.write(byte(button_release_cmd[i]));
            }
            break;
        case 0x14: // Standby
            cdc_active = false;
            
            // now that we're in standby, we don't want the display anymore
            display_wanted = false;
            
            /*for (int a = 0; a <=2; a++) {
             for (int j = 0; j < 8; j++) {
             CAN_TxMsg.id = SOUND_REQUEST;
             CAN_TxMsg.data[j] = beep_cmd[j];
             }
             CAN.send(&CAN_TxMsg);
             }
             */
            for (int j = 0; j < 8; j++) {
                Serial.write(byte(stopipod_cmd[j]));
            }
            delay(3);
            //Serial.println("Release");
            for (int i = 0; i < 7; i++) {
                Serial.write(byte(button_release_cmd[i]));
            }
            //Serial.println("Radio");
            break;
    }
    if (cdc_active) {
        switch (CAN_RxMsg.data[1]) {
            case 0x59: // next_cmd CD
                for (int j = 0; j < 7; j++) {
                    Serial.write(byte(playpauseipod_cmd[j]));
                }
                break;
            case 0x76: // Random on/off
                if (toggle_shuffle > 3) {
                    toggle_shuffle = 1;
                }
                switch (toggle_shuffle) {
                    case 1:
                        for (int j = 0; j < 9; j++) {
                            Serial.write(byte(repeat_cmd[j]));
                        }
                        break;
                    case 2:
                        for (int j = 0; j < 9; j++) {
                            Serial.write(byte(repeat_cmd[j]));
                        }
                        break;
                    case 3:
                        for (int j = 0; j < 9; j++) {
                            Serial.write(byte(repeat_cmd[j]));
                        }
                        for (int j = 0; j < 8; j++) {
                            Serial.write(byte(shuffle_cmd[j]));
                        }
                        break;
                }
                toggle_shuffle++;
                //break;
            case 0xB1: // Pause off
                for (int j = 0; j < 8; j++) {
                    Serial.write(byte(stopipod_cmd[j]));
                }
                break;
            case 0xB0: // Pause on
                for (int j = 0; j < 8; j++) {
                    Serial.write(byte(playipod_cmd[j]));
                }
                break;
            case 0x35: // Track up
                for (int j = 0; j < 7; j++) {
                    Serial.write(byte(next_cmd[j]));
                }
                break;
            case 0x36: // Track down
                for (int j = 0; j < 7; j++) {
                    Serial.write(byte(prev_cmd[j]));
                }
                break;
        }
        delay(3);
        //Serial.println("Release");
        for (int i = 0; i < 7; i++) {
            Serial.write(byte(button_release_cmd[i]));
        }
    }
}

/**
 * Handles the SID_BUTTONS frame.
 * TODO connect the SID button events to actions. For now, use Seth's original code:
 */

void CDCClass::handle_SID_buttons()
{
    if (!cdc_active) {
        return;
    }
    
    boolean event = (CAN_RxMsg.data[0] == 0x80);
    if (!event) {
        //FIXME: can we really ignore the message if it wasn't sent on event?
        return;
    }
    
    switch (CAN_RxMsg.data[2]) {
            //case 0x04: // NXT button on wheel
            //for (int j = 0; j < 9; j++) {
            //Serial.write(byte(repeat_cmd[j]));
            //}
            //break;
        case 0x10: // Seek+ button on wheel
            //Serial.println("next_cmd");
            for (int j = 0; j < 7; j++) {
                Serial.write(byte(next_cmd[j]));
            }
            break;
        case 0x08: // Seek- button on wheel
            //Serial.println("prev_cmd");
            for (int k = 0; k < 7; k++) {
                Serial.write(byte(prev_cmd[k]));
            }
            break;
    }
    delay(3);
    //Serial.println("Release");
    for (int i = 0; i < 7; i++) {
        Serial.write(byte(button_release_cmd[i]));
    }
}

/**
 * Puts the GENERAL_STATUS_CDC frame on the CAN bus
 */

void CDCClass::send_CDC_status(boolean event, boolean remote)
{
    CAN_TxMsg.id = GENERAL_STATUS_CDC;
    
    byte disc_mode = 0x05; // PLAY, or try 0x0e for "test mode" which might stop the IHU from updating the display?
    byte track_number = 0xFF;
    byte random_play = (0x00 << 5);
    byte disc_repeat = (0x00 << 1);
    byte track_repeat = 0x01;
    
    // byte 0, bit 7: FCI NEW DATA: 0 - sent on basetime, 1 - sent on event
    // byte 0, bit 6: FCI REMOTE CMD: 0 - status change due to internal operation, 1 - status change due to CDC_COMMAND frame
    // byte 0, bit 5: FCI DISC PRESENCE VALID: 0 - disc presence signal is not valid, 1 - disc presence signal is valid
    CAN_TxMsg.data[0] = (event ? 0x01 : 0x00) | (remote ? 0x02 : 0x04) << 5;
    
    // byte 1-2, bits 0-15: DISC PRESENCE: (bitmap) 0 - disc absent, 1 - disc present. Bit 0 is disc 1, bit 1 is disc 2, etc.
    CAN_TxMsg.data[1] = 0x01;
    CAN_TxMsg.data[2] = 0x01; // we have only one "disc" in the "magazine"
    
    // byte 3, bits 7-4: DISC MODE
    // byte 3, bits 3-0: DISC NUMBER
    CAN_TxMsg.data[3] = 0x01 | (disc_mode << 4) | 0x01;
    
    // byte 4: TRACK OR ERROR NUMBER
    CAN_TxMsg.data[4] = track_number;
    
    // byte 5: MINUTE
    CAN_TxMsg.data[5] = 0xFF;
    
    // byte 6: SECOND
    CAN_TxMsg.data[6] = 0xFF;
    
    // byte 7: CHANGER STATUS
    CAN_TxMsg.data[7] = 0xd0; // CDC is married to this vehicle, and there's a magazine present
    CAN_TxMsg.data[7] |= random_play | disc_repeat | track_repeat;
    
    // put the frame on the CAN bus:
    CAN.send(&CAN_TxMsg);
    
    // record the time of sending and reset status variables:
    cdc_status_last_send_time = millis();
    cdc_status_resend_needed = false;
    cdc_status_resend_due_to_cdc_command = false;
}



