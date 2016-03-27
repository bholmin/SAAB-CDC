
#ifndef RN52_H
#define RN52_H
#define SERIAL_BUFFER_SIZE      16      // Incomming buffer size
#define CMD_SEND_INTERVAL       500     // Interval in milliseconds; used for sending various commands to RN52 after conenction to it is established
#define BAUDRATE                9600    // RN52 is happier with 9600bps instead of default 115200bps when controlled by ATMEGA-328
#define BT_IDLE_TIME            300000  // Time in milliseconds that RN52 has been programmed with after which RN52 goes to sleep



/**
 * RN52 action command definitions:
 */

#define PLAYPAUSE               "AP"    // Play
#define NEXTTRACK               "AT+"   // Next Track
#define PREVTRACK               "AT-"   // Previous Track
#define CONNECT                 "B"     // Connect to last known device
#define DISCONNECT              "@,1"   // Disconnect from the current device and become "connectable"
#define REBOOT                  "R,1"   // Reboot RN52
#define VOLUP                   "AV+"   // Turn up the volume by one level
#define MAXVOLUME               "SS,0F" // Set the volume gain level to max
#define GETSTATUS               "Q"     // Querry the RN52 for status
#define ASSISTANT               "P"     // Invoke voice assistant


/**
 * Class:
 */

class RN52Class
{
    int serial_index;
    int volume_up_times_needed;
    int connection_attempts_remaining;
    int disconnection_attempts_remaining;
    bool response_received;
    bool waiting_for_status;
    bool status_connected;
    unsigned long response_timeout;
    unsigned long last_command_sent_time = 0;            // Timer to note down last time we sent a command to RN52
public:
    char in_buffer[SERIAL_BUFFER_SIZE];
    void initialize_atmel_pins();
    void wakeup();
    void uart_begin();
    void write(const char * in_message);
    bool read();
    void update();
    void start_connecting();
    void start_disconnecting();
    RN52Class() {
        serial_index = 0;
        response_received = true;
        waiting_for_status = false;
        status_connected = false;
        response_timeout = 0;
        volume_up_times_needed = 0;
        connection_attempts_remaining = 0;
        disconnection_attempts_remaining = 0;
    }
};

void turn_volume_to_max(void*);
void start_audio_playback(void*);
void finish_wakeup_procedure(void*);

/**
 * Variables:
 */

extern RN52Class RN52;

#endif