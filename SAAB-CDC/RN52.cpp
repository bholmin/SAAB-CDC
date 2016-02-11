
#include "Arduino.h"
#include "RN52.h"
#include "SoftwareSerial.h"
#include "Timer.h"



/**
 * Atmel 328 pin definitions:
 */

const int BT_STATUS_PIN = 3;    // RN52 GPIO2 pin for reading current status of the module
const int BT_CMD_PIN = 4;       // RN52 GPIO9 pin for enabling command mode
const int UART_TX_PIN = 5;      // UART Tx
const int UART_RX_PIN = 6;      // UART Rx
const int BT_RESET_PIN = 14;    // RN52 GPIO4 pin for factory reset
extern Timer time;              // Timer instance for timed actions

SoftwareSerial serial =  SoftwareSerial(UART_RX_PIN, UART_TX_PIN);

void RN52Class::initialize_atmel_pins() {
    pinMode(BT_CMD_PIN, OUTPUT);
    pinMode(BT_RESET_PIN,OUTPUT);
    digitalWrite(BT_CMD_PIN, HIGH);
    digitalWrite(BT_RESET_PIN, LOW);
}

void RN52Class::uart_begin() {
    serial.begin(115200);
    digitalWrite(BT_CMD_PIN,LOW);
}

void RN52Class::write(const char * in_message) {
    response_received = false;
    response_timeout = millis() + 10;
    serial.println(in_message);
    serial_index = 0;
    in_buffer[serial_index] = 0;
}

bool RN52Class::read() {
    if ((long)(millis() - response_timeout) >= 0) {
        response_received = true;
        return true;
    }
    while (serial.available() > 0) {
        char in_char = serial.read();
        #if (DEBUGMODE==1)
            Serial.println(int(in_char));
        #endif
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
    RN52.write(MAXVOLUME);
}

void start_audio_playback(void*) {
    RN52.write(PLAYPAUSE);
}

void RN52Class::update() {
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
            }
            else {
                write(DISCONNECT);
                disconnection_attempts_remaining--;
            }
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
}