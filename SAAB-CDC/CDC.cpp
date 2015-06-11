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
#define CDC_SID_FUNCTION_ID      30 // Decimal

/**
 * Other useful stuff
 */

#define MODULE_NAME              "BLUETOOTH"
#define DISPLAY_NAME_TIMEOUT     5000

/**
 * TX frames:
 */

#define GENERAL_STATUS_CDC       0x3c8
#define DISPLAY_RESOURCE_REQ     0x348 // 'Stolen' from the IHU since the CDC doesn't send this message
#define WRITE_TEXT_ON_DISPLAY    0x328 // 'Stolen' from the IHU since the CDC doesn't send this message
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

unsigned long cdc_status_last_send_time = 0; // Timer used to ensure we send the CDC status frame in a timely manner
unsigned long display_request_last_send_time = 0; // Timer used to ensure we send the display request frame in a timely manner
unsigned long write_text_on_display_last_send_time = 0; // Timer used to ensure we send the write text on display frame in a timely manner
unsigned long stop_displaying_name_at = 0; // Time at which we should stop displaying our name in the SID
boolean cdc_active = false; // True while our module, the simulated CDC, is active
boolean display_request_granted = true; // True while we are granted the 2nd row of the SID
boolean display_wanted = false; // True while we actually WANT the display
boolean cdc_status_resend_needed = false; // True if something has triggered the need to send the CDC status frame as an event
boolean cdc_status_resend_due_to_cdc_command = false; // True if the need for sending the CDC status frame was triggered by a CDC command
int power_pin = 7;
int play_pin = 5;
int forward_pin = 6;
int previous_pin = 8;
int spi_cs_pin = 16;
int spi_mosi_pin = 17;
int spi_miso_pin = 18;
int spi_sck_pin = 19;
int incomingByte = 0;   // For incoming serial data
int toggle_shuffle = 1; // TODO: switch to boolean?
int mute = 0; // TODO: switch to boolean?
int ninefive_cmd[] = {0x32,0x00,0x00,0x16,0x01,0x02,0x00,0x00};
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

/**
 * Opens CAN bus for communication.
 */

void CDCClass::open_CAN_bus()
{
    CAN.begin(47);  // SAAB I-Bus is 47.619kbps
    CAN_TxMsg.header.rtr = 0;     // This value never changes
    CAN_TxMsg.header.length = 8;  // This value never changes
}

/**
 * Handles CDC status and sends it to IHU as necessary.
 */

void CDCClass::handle_CDC_status()
{
    // If the CDC status frame needs to be sent as an event, do so now
    // (note though, that we may not send the frame more often than once every 50 ms)
    if (cdc_status_resend_needed && (millis() - cdc_status_last_send_time > 50)) {
        CDC.send_CDC_status(true, cdc_status_resend_due_to_cdc_command);
        Serial.println("DEBUG: Sending CDC status due to CDC command");
    }
    
    // The CDC status frame must be sent with a 1000 ms periodicity
    if (millis() - cdc_status_last_send_time > 900) {
        // Send the CDC status message, marked periodical and triggered internally
        CDC.send_CDC_status(false, false);
        Serial.println("DEBUG: Sending CDC status");
    }
}


/**
 * Prints the contents of CAN bus to serial console.
 */

void CDCClass::print_bus()
{
    if (CAN_RxMsg.id==0x348) {
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
//    digitalWrite(power_pin,HIGH);
//    delay(3000);
//    digitalWrite(power_pin, LOW);
    
    if (Serial.available() > 0) {
        incomingByte = Serial.read();
        Serial.print("DEBUG: 'incomingByte' =  ");
        Serial.println(incomingByte, DEC);
    }
}

/**
 * Handles an incoming (RX) frame.
 */

void CDCClass::handle_RX_frame()
{
    switch (CAN_RxMsg.id) {
        case NODE_STATUS_RX:
            // FIXME: We should check exactly what this network node message is and what the reply should be. For now, go with Seth's original code:
            CAN_TxMsg.id = NODE_STATUS_TX;
            for (int c = 0; c < 8; c++) {
                CAN_TxMsg.data[c] = ninefive_cmd[c];
            }
            CAN.send(&CAN_TxMsg);
            Serial.println("DEBUG: Received 'NODE_STATUS_RX' frame. Replying with '6A2'.");
            break;
            
        case CDC_CONTROL:
            handle_CDC_control();
            Serial.println("DEBUG: Received 'CDC_CONTROL' frame. Handling...");
            break;
            
        case SID_BUTTONS:
            handle_SID_buttons();
            Serial.println("DEBUG: Received 'SID_BUTTONS' frame. Handling...");
            break;
        /*
        case DISPLAY_RESOURCE_GRANT:
            if ((CAN_RxMsg.data[1] == 0x02) && (CAN_RxMsg.data[3] == CDC_SID_FUNCTION_ID)) {
                // we have been granted the right to write text to the second row in the SID
                Serial.println("SID");
                display_request_granted = true;
            }
            else if (CAN_RxMsg.data[1] == 0x02) {
                Serial.println("Someone else has been granted the second row, we need to back down");
                display_request_granted = false;
            }
            else if (CAN_RxMsg.data[1] == 0x00) {
                Serial.println("Someone else has been granted the entire display, we need to back down");
                display_request_granted = false;
            } 
            else {
                //someone else has been granted the first row; if we had the grant to the 2nd row, we still have it
            }
            break;
         */
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
            
            /*
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
            
            /*
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
        //FIXME: Can we really ignore the message if it wasn't sent on event?
        return;
    }
    
    switch (CAN_RxMsg.data[2]) {
        case 0x04: // NXT button on wheel
            //for (int j = 0; j < 9; j++) {
            //Serial.write(byte(repeat_cmd[j]));
            //}
            Serial.println("DEBUG: 'NXT' button on wheel pressed.");
            break;
        case 0x10: // Seek+ button on wheel
            //Serial.println("next_cmd");
            //for (int j = 0; j < 7; j++) {
            //Serial.write(byte(next_cmd[j]));
            //}
            Serial.println("DEBUG: 'Seek+' button on wheel pressed.");
            break;
        case 0x08: // Seek- button on wheel
            //Serial.println("prev_cmd");
            //for (int k = 0; k < 7; k++) {
            //Serial.write(byte(prev_cmd[k]));
            //}
            Serial.println("DEBUG: 'Seek-' button on wheel pressed.");
            break;
    }
    delay(3);
    Serial.println("DEBUG: 'Button Release' command sent.");
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
    
    CAN_TxMsg.data[0] = 0xE0;
    
    // byte 1-2, bits 0-15: DISC PRESENCE: (bitmap) 0 - disc absent, 1 - disc present. Bit 0 is disc 1, bit 1 is disc 2, etc.
    CAN_TxMsg.data[1] = 0xFF;
    CAN_TxMsg.data[2] = 0x01; // we have only one "disc" in the "magazine"
    
    // byte 3, bits 7-4: DISC MODE
    // byte 3, bits 3-0: DISC NUMBER
    // CAN_TxMsg.data[3] = 0x05 // PLAY
    CAN_TxMsg.data[3] = 0x31;
    
    // byte 4: TRACK OR ERROR NUMBER
    CAN_TxMsg.data[4] = 0xFF;
    
    // byte 5: MINUTE
    CAN_TxMsg.data[5] = 0xFF;
    
    // byte 6: SECOND
    CAN_TxMsg.data[6] = 0xFF;
    
    // byte 7: CHANGER STATUS
    CAN_TxMsg.data[7] = 0xD0; // CDC is married to this vehicle, and there's a magazine present
    
    // put the frame on the CAN bus:
    CAN.send(&CAN_TxMsg);
    
    // Record the time of sending and reset status variables
    cdc_status_last_send_time = millis();
    cdc_status_resend_needed = false;
    cdc_status_resend_due_to_cdc_command = false;
    
}