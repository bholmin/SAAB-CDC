//
//  Project 	 SAAB-CDC
//
//  Code:        Seth Evans and Emil Malmberg
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

#define MODULE_NAME              "BT TEST"
#define DISPLAY_NAME_TIMEOUT     5000
#define BT_POWER_TIMEOUT         3000
#define BT_PIN_TIMEOUT           50

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

unsigned long cdc_status_last_send_time = 0; // Timer used to ensure we send the CDC status frame in a timely manner
unsigned long display_request_last_send_time = 0; // Timer used to ensure we send the display request frame in a timely manner
unsigned long write_text_on_display_last_send_time = 0; // Timer used to ensure we send the write text on display frame in a timely manner
unsigned long stop_displaying_name_at = 0; // Time at which we should stop displaying our name in the SID
boolean cdc_active = false; // True while our module, the simulated CDC, is active
boolean display_request_granted = true; // True while we are granted the 2nd row of the SID
boolean display_wanted = false; // True while we actually WANT the display
boolean cdc_status_resend_needed = false; // True if something has triggered the need to send the CDC status frame as an event
boolean cdc_status_resend_due_to_cdc_command = false; // True if the need for sending the CDC status frame was triggered by a CDC command
boolean bt_off = true; // Default state of bluetooth module
boolean bt_on_pairing = false; // True while bluetooth module is on and is in pairing mode
boolean bt_on_active = false; // True while bluetooth module is on and is paired to a device
boolean mute = false;
int power_pin = 7;
int play_pin = 5;
int forward_pin = 6;
int previous_pin = 8;
int spi_cs_pin = 16;
int spi_mosi_pin = 17;
int spi_miso_pin = 18;
int spi_sck_pin = 19;
int incomingByte = 0;   // For incoming serial data
int toggle_shuffle = 1;
int ninefive_cmd[] = {0x32,0x00,0x00,0x16,0x01,0x02,0x00,0x00,-1};
int beep_cmd[] = {0x80,0x04,0x00,0x00,0x00,0x00,0x00,0x00,-1};
int playipod_cmd[] = {0xFF,0x55,0x04,0x02,0x00,0x00,0x01,0xF9,-1};
int playpauseipod_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x01,0xFA,-1};
int stopipod_cmd[] = {0xFF,0x55,0x04,0x02,0x00,0x00,0x02,0xF8,-1};
int next_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x08,0xF3,-1};
int prev_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x10,0xEB,-1};
int shuffle_cmd[] = {0xFF,0x55,0x04,0x02,0x00,0x00,0x80,0x7A,-1};
int repeat_cmd[] = {0xFF,0x55,0x05,0x02,0x00,0x00,0x00,0x01,0xF8,-1};
int button_release_cmd[] = {0xFF,0x55,0x03,0x02,0x00,0x00,0xFB,-1};
int cdc_status_cmd[] = {0xE0,0xFF,0x01,0x31,0xFF,0xFF,0xFF,0xD0,-1};
int display_request_cmd[] = {CDC_APL_ADR,2,2,CDC_SID_FUNCTION_ID,0x00,0x00,0x00,0x00,-1};

/******************************************************************************
 * PUBLIC METHODS
 ******************************************************************************/

/**
 * Prints the CAN message to serial output.
 */

void CDCClass::print_can_message() {
    Serial.print(CAN_TxMsg.id,HEX);
    Serial.print(" -> ");
    for (int i = 0; i < 8; i++) {
        Serial.print(CAN_TxMsg.data[i],HEX);
        Serial.print(" ");
    }
    Serial.println();
}

/**
 * Initializes pins on ATMEGA328-PU chip for further use with bluetooth module.
 */

void CDCClass::initialize_BT_pins() {
    pinMode(power_pin, OUTPUT);
    pinMode(play_pin, OUTPUT);
    pinMode(forward_pin, OUTPUT);
    pinMode(previous_pin, OUTPUT);
    
    digitalWrite(power_pin,LOW);
    digitalWrite(play_pin,LOW);
    digitalWrite(forward_pin,LOW);
    digitalWrite(previous_pin,LOW);
}


/**
 * Opens CAN bus for communication.
 */

void CDCClass::open_CAN_bus() {
    //Serial.println("DEBUG: Opening CAN bus @ 47.619kbps");
    CAN.begin(47);  // SAAB I-Bus is 47.619kbps
    CAN_TxMsg.header.rtr = 0;     // This value never changes
    CAN_TxMsg.header.length = 8;  // This value never changes
}

/**
 * Handles actions with the BC05B Bluetooth module.
 */

void CDCClass::handle_BT_connection(byte action) {
    switch (action) {
        case 'P':
            digitalWrite(power_pin,HIGH);
            delay(BT_POWER_TIMEOUT);
            digitalWrite(power_pin,LOW);
            break;
        case 'F':
            digitalWrite(forward_pin,HIGH);
            delay(BT_PIN_TIMEOUT);
            digitalWrite(forward_pin,LOW);
            break;
        case 'R':
            digitalWrite(previous_pin,HIGH);
            delay(BT_PIN_TIMEOUT);
            digitalWrite(previous_pin,LOW);
            break;
    }
}

/**
 * Testing of comms with BC05B Bluetooth module.
 */

void CDCClass::test_bt() {
    if (Serial.available() > 0) {
        int incomingByte = Serial.read();
        switch (incomingByte) {
            case 'P':
                digitalWrite(power_pin,HIGH);
                Serial.println("Power pin HIGH");
                delay(BT_POWER_TIMEOUT);
                digitalWrite(power_pin,LOW);
                Serial.println("Power pin LOW");
                break;
            case 'F':
                digitalWrite(forward_pin,HIGH);
                Serial.println("Forward pin HIGH");
                delay(BT_PIN_TIMEOUT);
                digitalWrite(forward_pin,LOW);
                Serial.println("Forward pin LOW");
                break;
            case 'R':
                digitalWrite(previous_pin,HIGH);
                Serial.println("Previous pin HIGH");
                delay(BT_PIN_TIMEOUT);
                digitalWrite(previous_pin,LOW);
                Serial.println("Previous pin LOW");
                break;
        }
    }
}



/**
 * Handles an incoming (RX) frame.
 */

void CDCClass::handle_RX_frame() {
    if (CAN.CheckNew()) {
        CAN_TxMsg.data[0]++;
        CAN.ReadFromDevice(&CAN_RxMsg);
        switch (CAN_RxMsg.id) {
            case NODE_STATUS_RX:
                send_can_message(NODE_STATUS_TX, ninefive_cmd);
                //Serial.println("DEBUG: Received 'NODE_STATUS_RX' frame. Replying with '6A2'.");
                break;
            case IHU_BUTTONS:
                handle_IHU_buttons();
                //Serial.println("DEBUG: Received 'IHU_BUTTONS' frame. Handling...");
                break;
            case STEERING_WHEEL_BUTTONS:
                handle_steering_wheel_buttons();
                //Serial.println("DEBUG: Received 'STEERING_WHEEL_BUTTONS' frame. Handling...");
                break;
            case DISPLAY_RESOURCE_GRANT:
                if ((CAN_RxMsg.data[1] == 0x02) && (CAN_RxMsg.data[3] == CDC_SID_FUNCTION_ID)) {
                    //Serial.println("DEBUG: We have been granted the right to write text to the second row in the SID.");
                    display_request_granted = true;
                }
                else if (CAN_RxMsg.data[1] == 0x02) {
                    //Serial.println("DEBUG: Someone else has been granted the second row, we need to back down");
                    display_request_granted = false;
                }
                else if (CAN_RxMsg.data[1] == 0x00) {
                    //Serial.println("DEBUG: Someone else has been granted the entire display, we need to back down");
                    display_request_granted = false;
                }
                else {
                    //Serial.println("DEBUG: Someone else has been granted the first row; if we had the grant to the 2nd row, we still have it.");
                }
                break;
        }
    }
}

/**
 * Handles the IHU_BUTTONS frame that the IHU sends us when it wants to control some feature of the CDC.
 */

void CDCClass::handle_IHU_buttons() {
    boolean event = (CAN_RxMsg.data[0] == 0x80);
    if (!event) {
        // FIXME: can we really ignore the message if it wasn't sent on event?
        return;
    }
    switch (CAN_RxMsg.data[1]) {
        case 0x24: // Play
            cdc_active = true;
            // We want to show the module name in the SID for a little while after being activated:
            stop_displaying_name_at = millis() + DISPLAY_NAME_TIMEOUT;
            display_wanted = true;
            send_can_message(SOUND_REQUEST, beep_cmd);
            send_serial_message(playipod_cmd);
            //Serial.println("DEBUG: 'Button Release' command sent.")
            send_serial_message(button_release_cmd);
            break;
        case 0x14: // Standby
            cdc_active = false;
            /*
            // Wow that we're in standby, we don't want the display anymore
            display_wanted = false;
            for (int a = 0; a <=2; a++) {
                send_can_message(SOUND_REQUEST, beep_cmd);
            }
             */
            send_serial_message(stopipod_cmd);
            send_serial_message(button_release_cmd);
            break;
    }
    if (cdc_active) {
        switch (CAN_RxMsg.data[1]) {
            case 0x59: // Next_cmd CD
                send_serial_message(playpauseipod_cmd);
                handle_BT_connection('P');
                break;
            case 0x76: // Random ON/OFF
                if (toggle_shuffle > 4) {
                    toggle_shuffle = 1;
                }
                switch (toggle_shuffle) {
                    case 1:
                        send_serial_message(repeat_cmd);
                        break;
                    case 2:
                        send_serial_message(repeat_cmd);
                        break;
                    case 3:
                        send_serial_message(repeat_cmd);
                        break;
                    case 4:
                        send_serial_message(shuffle_cmd);
                        break;
                }
                toggle_shuffle++;
            case 0xB1: // Pause ON
                send_serial_message(stopipod_cmd);
                break;
            case 0xB0: // Pause OFF
                send_serial_message(playipod_cmd);
                break;
            case 0x35: // Track up
                send_serial_message(next_cmd);
                handle_BT_connection('F');
                break;
            case 0x36: // Track down
                send_serial_message(prev_cmd);
                handle_BT_connection('R');
                break;
            default:
                Serial.print(CAN_RxMsg.data[1],HEX);
                //Serial.println("DEBUG: Unknown (for now) message");
        }
        //Serial.println("DEBUG: 'Button Release' command sent.")
        send_serial_message(button_release_cmd);
    }
}

/**
 * Handles the STEERING_WHEEL_BUTTONS frame.
 * TODO connect the SID button events to actions.
 */

void CDCClass::handle_steering_wheel_buttons() {
    if (!cdc_active) {
        return;
    }
    boolean event = (CAN_RxMsg.data[0] == 0x80);
    if (!event) {
        // FIXME: Can we really ignore the message if it wasn't sent on event?
        return;
    }
    switch (CAN_RxMsg.data[2]) {
        case 0x04: // NXT button on wheel
            //send_serial_message(repeat_cmd);
            //Serial.println("DEBUG: 'NXT' button on wheel pressed.");
            break;
        case 0x10: // Seek+ button on wheel
            //send_serial_message(next_cmd);
            //Serial.println("DEBUG: 'Seek+' button on wheel pressed.");
            break;
        case 0x08: // Seek- button on wheel
            //send_serial_message(prev_cmd);
            //Serial.println("DEBUG: 'Seek-' button on wheel pressed.");
            break;
        default:
            //Serial.print(CAN_RxMsg.data[2],HEX);
            //Serial.println("DEBUG: Unknown SID button message");
            break;
    }
    //Serial.println("DEBUG: 'Button Release' command sent.");
    send_serial_message(button_release_cmd);
}

/**
 * Handles CDC status and sends it to IHU as necessary.
 */

void CDCClass::handle_CDC_status() {
    
    handle_RX_frame();
    
    // If the CDC status frame needs to be sent as an event, do so now
    // (note though, that we may not send the frame more often than once every 50 ms)
    
    if (cdc_status_resend_needed && (millis() - cdc_status_last_send_time > 50)) {
        send_CDC_status(true, cdc_status_resend_due_to_cdc_command);
        //Serial.println("DEBUG: Sending CDC status due to CDC command");
    }
    
    // The CDC status frame must be sent with a 1000 ms periodicity
    
    if (millis() - cdc_status_last_send_time > 950) {
        // Send the CDC status message, marked periodical and triggered internally
        send_CDC_status(false, false);
        //Serial.println("DEBUG: Sending CDC status");
    }
}

void CDCClass::send_CDC_status(boolean event, boolean remote) {
    send_can_message(GENERAL_STATUS_CDC, cdc_status_cmd);
    
    // Record the time of sending and reset status variables
    cdc_status_last_send_time = millis();
    cdc_status_resend_needed = false;
    cdc_status_resend_due_to_cdc_command = false;

}

/**
 * Sends a request for using the SID, row 2. We may NOT start writing until we've received a grant frame with the correct function ID!
 */

void CDCClass::send_display_request() {
    send_can_message(DISPLAY_RESOURCE_REQ, display_request_cmd);
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
    //delay(3); // Not sure if this is needed. Seems to be working fine without this.
}

/**
 * Formats and puts a frame on CAN bus
 */

void CDCClass::send_can_message(int message_id, int *msg) {
    CAN_TxMsg.id = message_id;
    int i = 0;
    while (msg[i] != -1) {
        CAN_TxMsg.data[i] = msg[i];
        i++;
    }
    CAN.send(&CAN_TxMsg);
    //print_can_message();
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
    
    Serial.print("n= ");
    Serial.println(n, DEC);
    Serial.print("txt ");
    Serial.println(txt);
    Serial.print("text ");
    Serial.println(text);
    
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