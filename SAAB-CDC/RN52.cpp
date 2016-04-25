
#include "Arduino.h"
#include "RN52.h"
#include "SoftwareSerial.h"
#include "Timer.h"



/**
 * Atmel 328 pin definitions:
 */

const int BT_STATUS_PIN = 3;    // RN52 GPIO2 pin for reading current status of the module
const int BT_CMD_PIN = 4;       // RN52 GPIO9 pin for enabling command mode
const int BT_FACT_RST_PIN = A0; // RN52 factory reset pin GPIO4
const int BT_PWREN_PIN = 9;     // RN52 Power enable pin
const int UART_TX_PIN = 5;      // UART Tx
const int UART_RX_PIN = 6;      // UART Rx
extern Timer time;              // Timer instance for timed actions

SoftwareSerial serial =  SoftwareSerial(UART_RX_PIN, UART_TX_PIN);

void RN52Class::initialize_atmel_pins() {
    pinMode(BT_STATUS_PIN,INPUT);
    pinMode(BT_CMD_PIN,OUTPUT);
    pinMode(BT_FACT_RST_PIN,INPUT); // Some REALLY crazy stuff is going on if this pin is set as output and pulled low. Leave it alone! Trust me...
    pinMode(BT_PWREN_PIN,OUTPUT);
    digitalWrite(BT_CMD_PIN,HIGH);
}

void RN52Class::wakeup() {
    int pin_status;
     
    Serial.println("RN52: \"Starting wakeup procedure\"");
    
    pin_status = digitalRead(BT_PWREN_PIN);
    Serial.print("RN52: \"Status of BT_PWREN_PIN: ");
    Serial.print(pin_status);
    Serial.println("\"");
    
    digitalWrite(BT_PWREN_PIN,HIGH);
    
    pin_status = digitalRead(BT_PWREN_PIN);
    Serial.print("RN52: \"Status of BT_PWREN_PIN: ");
    Serial.print(pin_status);
    Serial.println("\"");
    time.after(3000,finish_wakeup_procedure,NULL);
    
    if ((millis() - last_command_sent_time) < BT_IDLE_TIME) {
        Serial.println("RN52: \"Module seems to be awake. No need to wakeup (PWREN). Sending CONNECT command\. Don't trust this message entirely though :)\"");
        write(CONNECT);
    }
}

void RN52Class::uart_begin() {
    int pin_status;
    Serial.print("RN52: \"Opening software serial connection @ ");
    Serial.print(BAUDRATE);
    Serial.println(" bps\"");
    serial.begin(BAUDRATE);
    digitalWrite(BT_CMD_PIN,LOW);
    pin_status = digitalRead(BT_CMD_PIN);
    Serial.print("RN52: \"Status of BT_CMD_PIN: ");
    Serial.print(pin_status);
    Serial.println("\"");
}

void RN52Class::write(const char * in_message) {
    response_received = false;
    response_timeout = millis() + 10;
    serial.println(in_message);
    serial_index = 0;
    in_buffer[serial_index] = 0;
    last_command_sent_time = millis();
}

bool RN52Class::read() {
    if ((long)(millis() - response_timeout) >= 0) {
        response_received = true;
        return true;
    }
    while (serial.available() > 0) {
        char in_char = serial.read();
        if (in_char == '\r') continue;
        if (in_char == '\n') {
            response_received = true;
            in_buffer[serial_index] = 0;
            serial_index = 0;
            return true;
        }
        if (serial_index < SERIAL_RX_BUFFER_SIZE - 1) {
            in_buffer[serial_index] = in_char;
            serial_index++;
            in_buffer[serial_index] = 0;
        }
    }
    return false;
}

void RN52Class::start_connecting() {
    connection_attempts_remaining = 2;
}

void RN52Class::start_disconnecting() {
    disconnection_attempts_remaining = 2;
}

void turn_volume_to_max(void*) {
    Serial.println("RN52: \"Turning up the volume of RN52 to max\"");
    RN52.write(MAXVOLUME);
}

void start_audio_playback(void*) {
    Serial.println("RN52: \"Starting auto play\"");
    RN52.write(PLAYPAUSE);
}

void finish_wakeup_procedure(void*) {
    int pin_status;
    
    Serial.println("RN52: \"Finishing RN52 \"wakeup\" procedure\"");
    
    digitalWrite(BT_PWREN_PIN,LOW);
    
    pin_status = digitalRead(BT_PWREN_PIN);
    Serial.print("RN52: \"Status of BT_PWREN_PIN: ");
    Serial.print(pin_status);
    Serial.println("\"");
    
    RN52.uart_begin();
    
    // time.after(CMD_SEND_INTERVAL,turn_volume_to_max,NULL);
    time.after(CMD_SEND_INTERVAL * 2,start_audio_playback,NULL);
}

void RN52Class::monitor_serial_input() {
    int incomingByte = 0;
    
    if (Serial.available() > 0) {
        incomingByte = Serial.read();
        switch (incomingByte) {
            case 'W':
                Serial.println("RN52: \"Manual wakeup.");
                RN52.wakeup();
                break;
            case 'C':
                Serial.println("RN52: \"Manual reconnect to last known device.");
                RN52.write(CONNECT);
                break;
            case 'D':
                Serial.println("RN52: \"Manual disconnect from device.");
                RN52.write(DISCONNECT);
            case 'R':
                Serial.println("RN52: \"Manual reboot.");
                RN52.write(REBOOT);
                break;
            default:
                break;
        }
    }
}

void RN52Class::update() {
    int bt_status_pin_state;
    bt_status_pin_state = digitalRead(BT_STATUS_PIN);
    
    if (bt_status_pin_state == 0) {
        read();
       // Serial.println("RN52: \"State of RN52 has changed. Reading status...\"");
    }
    
    /*
    read();
    
    if (response_received) {
        if (waiting_for_status) {
            if (strlen(in_buffer) != 4) {
                write(GETSTATUS);
            }
            else {
                waiting_for_status = false;
                status_connected = (in_buffer[3] >= '3');
            }
        }
        
        if (disconnection_attempts_remaining > 0) {
            if (!status_connected) {
                disconnection_attempts_remaining = 0;
                connection_handled = false;
            }
            else {
                write(DISCONNECT);
                disconnection_attempts_remaining--;
            }
        }
        
        else if ((status_connected) && (!connection_handled)) {
            connection_handled = true;
            time.after(CMD_SEND_INTERVAL,turn_volume_to_max,NULL);
            time.after(CMD_SEND_INTERVAL * 2,start_audio_playback,NULL);
        }
     
        else if (connection_attempts_remaining > 0) {
            if (status_connected) {
                connection_attempts_remaining = 0;
            }
            else {
                write(CONNECT);
                connection_attempts_remaining--;
            }
            if (connection_attempts_remaining == 0) {
                time.after(CMD_SEND_INTERVAL,turn_volume_to_max,NULL);
                time.after(CMD_SEND_INTERVAL * 2,start_audio_playback,NULL);
            }
        }


        else if (digitalRead(BT_STATUS_PIN) == LOW) {
            write(GETSTATUS);
            waiting_for_status = true;
        }
    }
     */
}