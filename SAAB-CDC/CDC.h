//
// Project 		SAAB-CDC
//
//
//

#ifndef CDC_H
#define CDC_H

#include "Arduino.h"
#include "CAN.h"
#include "SPI.h"

//----------------------------------------------------------------------------
// CLASS
//----------------------------------------------------------------------------

class CDCClass
{
public:
    void print_can_frame();
    void initialize_atmel_pins();
    void open_can_bus();
    void handle_bt_connection(int pin);
    void test_bt();
    void handle_rx_frame();
    void handle_ihu_buttons();
    void handle_steering_wheel_buttons();
    void handle_cdc_status();
    void send_cdc_status(boolean event, boolean remote);
    void send_display_request();
    void send_serial_message(int *msg);
    void send_can_frame(int message_id, int *msg);
    void write_text_on_display(char text[]);
};

//----------------------------------------------------------------------------
// VARIABLES
//----------------------------------------------------------------------------

extern CDCClass CDC;

#endif
