
#ifndef CDC_H
#define CDC_H
#define DEBUGMODE                0     // 1 = Output debug to serial port; 0 = No output
#define CDC_STATUS_TX_TIME       1000  // The CDC status frame must be sent with a 1000 ms periodicity.

//----------------------------------------------------------------------------
// CLASS
//----------------------------------------------------------------------------

class CDCClass
{
public:
    void print_can_tx_frame();
    void print_can_rx_frame();
    void open_can_bus();
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

void send_node_status();
void send_cdc_status_on_time(void*);

//----------------------------------------------------------------------------
// VARIABLES
//----------------------------------------------------------------------------

extern CDCClass CDC;

#endif