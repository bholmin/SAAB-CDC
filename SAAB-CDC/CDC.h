//
// Project 		SAAB-CDC
//
//
//

#ifndef CDC_H
#define CDC_H

#include "Arduino.h"
#include "CAN.h"

//----------------------------------------------------------------------------
// CLASS
//----------------------------------------------------------------------------

class CDCClass
{
public:
    void print_can_message();
    void open_CAN_bus();
    void handle_BT_connection();
    void handle_RX_frame();
    void handle_IHU_buttons();
    void handle_steering_wheel_buttons();
    void handle_CDC_status();
    void send_CDC_status(boolean event, boolean remote);
    void send_display_request();
    void send_serial_message(int *msg);
    void send_can_message(int message_id, int *msg);
    void write_text_on_display(char text[]);
};

//----------------------------------------------------------------------------
// VARIABLES
//----------------------------------------------------------------------------

extern CDCClass CDC;

#endif
