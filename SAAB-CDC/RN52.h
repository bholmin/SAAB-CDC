
#ifndef RN52_H
#define RN52_H
#define SERIAL_BUFFER_SIZE 10
#define CMD_SEND_INTERVAL 500 // Interval in milliseconds. Used for sending various commands to RN52 after conenction to it is established.
#define DEBUGMODE 0 // 1 = Output debug to serial port; 0 = No output

// RN52 action command definitions

#define PLAYPAUSE   "AP"
#define NEXTTRACK   "AT+"
#define PREVTRACK   "AT-"
#define CONNECT     "B"
#define DISCONNECT  "@,1"
#define REBOOT      "R,1"
#define VOLUP       "AV+"
#define MAXVOLUME   "SS,0F"
#define GETSTATUS   "Q"
#define ASSISTANT   "P"

//----------------------------------------------------------------------------
// CLASS
//----------------------------------------------------------------------------

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
public:
    char in_buffer[SERIAL_BUFFER_SIZE];
    void initialize_atmel_pins();
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

//----------------------------------------------------------------------------
// VARIABLES
//----------------------------------------------------------------------------

extern RN52Class RN52;

#endif