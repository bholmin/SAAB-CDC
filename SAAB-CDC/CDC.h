/*
 * C++ Class for handling CD changer emulator on SAAB I-Bus
 * Copyright (C) 2016  Karlis Veilands
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Created by: Karlis Veilands
 * Created on: Jun 4, 2015
 * Modified by: Karlis Veilands
 * Modified on: May 17, 2016
 */

#ifndef CDC_H
#define CDC_H



/**
 * Various constants used for identifying the CDC
 */

#define CDC_APL_ADR                 0x12
#define CDC_SID_FUNCTION_ID         30      // Decimal

/**
 * Other useful stuff
 */

#define MODULE_NAME                 "BT TEST"
#define LAST_EVENT_IN_TIMEOUT       3000    // Milliseconds
#define DISPLAY_NAME_TIMEOUT        5000    // Milliseconds
#define NODE_STATUS_TX_MSG_SIZE     4       // Decimal; defines how many frames do we need to reply with to '6A1'

/**
 * TX frames:
 */

#define GENERAL_STATUS_CDC          0x3C8
#define DISPLAY_RESOURCE_REQ        0x348   // 'Stolen' from the IHU since the CDC doesn't send this frame
#define WRITE_TEXT_ON_DISPLAY       0x328   // 'Stolen' from the IHU since the CDC doesn't send this frame
#define NODE_STATUS_TX              0x6A2
#define SOUND_REQUEST               0x430

/**
 * RX frames:
 */

#define IHU_BUTTONS                 0x3C0
#define DISPLAY_RESOURCE_GRANT      0x368
#define NODE_STATUS_RX              0x6A1
#define STEERING_WHEEL_BUTTONS      0x290

/**
 * Timer definitions:
 */

#define NODE_STATUS_TX_TIME         140     // Replies to '6A1' request need to be sent with no more than 140ms interval
#define CDC_STATUS_TX_TIME          950     // The CDC status frame must be sent periodically within this timeframe

/**
 * SID sound type definitions:
 */

#define SOUND_ACK                   0x04    // Short "Beep"
#define SOUND_TAC                   0x08    // "Tack"
#define SOUND_TIC                   0x10    // "Tick"
#define SOUND_ALERT                 0x40    // Short "Ding-Dong"

/**
 * Class:
 */

class CDChandler {
public:
    void printCanTxFrame();
    void printCanRxFrame();
    void openCanBus();
    void handleRxFrame();
    void handleIhuButtons();
    void handleSteeringWheelButtons();
    void handleCdcStatus();
    void sendCdcStatus(boolean event, boolean remote);
    void sendDisplayRequest();
    void sendCanFrame(int message_id, int *msg);
    void writeTextOnDisplay(char text[]);
    void checkCanEvent(int frameElement);
};

void sendCdcStatusOnTime(void*);

/**
 * Variables:
 */

extern CDChandler CDC;

#endif