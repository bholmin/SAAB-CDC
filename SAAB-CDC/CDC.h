
#ifndef CDC_H
#define CDC_H

/**
 * Various constants used for identifying the CDC
 */

#define CDC_APL_ADR              0x12
#define CDC_SID_FUNCTION_ID      30   // Decimal

/**
 * Other useful stuff
 */

#define MODULE_NAME              "BT TEST"
#define LAST_EVENT_IN_TIMEOUT    2000 // Milliseconds
#define DISPLAY_NAME_TIMEOUT     5000 // Milliseconds
#define NODE_STATUS_TX_MSG_SIZE  4    // Decimal. Defines how many messages do we need to reply with to '6A1'
#define DEBUGMODE                0    // 1 = Output debug to serial port; 0 = No output

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
 * Timer definitions:
 */

#define NODE_STATUS_TX_TIME      140    // Replies to '6A1' request need to be sent with no more than 140ms interval
#define CDC_STATUS_RE_TX_TIME    50     // Interval for CDC status frame if it needs to be sent as an event
#define CDC_STATUS_TX_TIME       1000   // The CDC status frame must be sent with a 1000ms interval periodicity

/**
 * SID Sound type definitions:
 */

#define SOUND_ACK                0x04   // Short "Beep"
#define SOUND_TAC                0x08   // "Tack"
#define SOUND_TIC                0x10   // "Tic"
#define SOUND_ALERT              0x40   // "Ding-Dong"


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