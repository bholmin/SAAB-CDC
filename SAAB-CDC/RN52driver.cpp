/*
 * Virtual C++ Class for RovingNetworks RN-52 Bluetooth modules
 * Copyright (C) 2013  Tim Otto
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
 * Created by: Tim Otto
 * Created on: Jun 21, 2013
 * Modified by: Karlis Veilands
 * Modified on: May 17, 2016
 */

#include <Arduino.h>
#include <string.h>
#include "RN52driver.h"
#include "RN52strings.h"

using namespace std;

namespace RN52 {
    
    static int getVal(char c);
    
    RN52driver::RN52driver() :
    mode(DATA), enterCommandMode(false), enterDataMode(false), state(0), profile(0), a2dpConnected(false),
    sppConnected(false), streamingAudio(false), sppTxBufferPos(0), cmdRxBufferPos(0), currentCommand(NULL), commandQueuePos(0)
    {}
    
    int RN52driver::fromUART(const char c)
    {
        if (mode == DATA && !enterCommandMode) {
            fromSPP(&c, 1);
            return 1;
        } else {
            return parseCmdResponse(&c, 1);
            
        }
    }
    
    int RN52driver::fromUART(const char *data, int size)
    {
        if (mode == DATA && !enterCommandMode) {
            fromSPP(data, size);
        } else {
            int parsed = parseCmdResponse(data, size);
            if (parsed < size && parsed > 0) {
                // Not all UART bytes were processed by the CommandMode parser.
                // This means there was an END\r\n, so feed the remainder to SPP
                if (mode == DATA) {
                    fromSPP(data + parsed, size - parsed);
                } else {
                    onError(1, OVERFLOW);
                    return -1;
                }
            }
        }
        return size;
    }
    
    int RN52driver::toSPP(const char c)
    {
        if(!sppConnected) {
            onError(2, NOTCONNECTED);
            return -2;
        }
        
        if (mode == DATA && !enterCommandMode)
            toUART(&c, 1);
        else {
            // in command mode, buffer
            int left = SPP_TX_BUFFER_SIZE - sppTxBufferPos;
            if (left < 1) {
                onError(2, OVERFLOW);
                return -1;
            }
            
            sppTxBuffer[sppTxBufferPos++] = c;
        }
        return 1;
    }
    
    int RN52driver::toSPP(const char *data, int size)
    {
        if(!sppConnected) {
            onError(3, NOTCONNECTED);
            return -2;
        }
        
        if (mode == DATA && !enterCommandMode)
            toUART(data, size);
        else {
            // in command mode, buffer
            int left = SPP_TX_BUFFER_SIZE - sppTxBufferPos;
            if (left < size) {
                onError(3, OVERFLOW);
                return -1;
            }
            
            memcpy(sppTxBuffer+sppTxBufferPos, data, size);
            sppTxBufferPos += size;
        }
        return size;
    }
    
    static bool isCmd(const char *buffer, const char *cmd) {
        return strncmp(buffer, cmd, strlen(cmd)) == 0;
    }
    
    int RN52driver::parseCmdResponse(const char *data, int size)
    {
        int parsed = 0;
        while (parsed < size) {
            if (cmdRxBufferPos == CMD_RX_BUFFER_SIZE) {
                onError(4, OVERFLOW);
                return -1;
            }
            
            cmdRxBuffer[cmdRxBufferPos++] = data[parsed++];
            if (mode == DATA) {
                // did not receive CMD\r\n yet
                if (cmdRxBufferPos == 5) {
                    if (isCmd(cmdRxBuffer, RN52_CMD_BEGIN)) {
                        mode = COMMAND;
                        enterCommandMode = false;
                        cmdRxBufferPos = 0;
                    } else {
                        toSPP(cmdRxBuffer[0]);
                        for (int i = 1; i < 5; i++)
                            cmdRxBuffer[i - 1] = cmdRxBuffer[i];
                        cmdRxBufferPos--;
                    }
                }
            } else if (cmdRxBufferPos >= 2) {
                if (cmdRxBuffer[cmdRxBufferPos - 1] == '\n' && cmdRxBuffer[cmdRxBufferPos - 2] == '\r') {
                    // this is a line
                    if (isCmd(cmdRxBuffer, RN52_CMD_EXIT)) {
                        mode = DATA;
                        cmdRxBufferPos = 0;
                        enterDataMode = false;
                        
                        if (commandQueuePos > 0) {
                            // there's outgoing command requests (yet or again)
                            prepareCommandMode();
                        }
                        break;
                    }
                    // TODO handle other responses, depending on the command sent before
                    if (currentCommand == NULL) {
                        cmdRxBuffer[cmdRxBufferPos - 2] = 0;
                    } else if (isCmd(currentCommand, RN52_CMD_QUERY)) {
                        parseQResponse(cmdRxBuffer);
                        currentCommand = NULL;
                    } else if (isCmd(currentCommand, RN52_CMD_DETAILS)) {
                        // multiple lines
                        //TODO set currentCommand to NULL after the 10th line of response
                        currentCommand = NULL;
                    } else {
                        // misc command (AVCRP, connect/disconnect, etc)
                        if (isCmd(cmdRxBuffer, RN52_RX_OK)) {
                            // OK
                        } else if (isCmd(cmdRxBuffer, RN52_RX_ERROR)) {
                            // Error
                        } else if (isCmd(cmdRxBuffer, RN52_RX_WHAT)) {
                            // WTF!?
                        } else {
                            cmdRxBuffer[cmdRxBufferPos - 2] = 0;
                            onError(4, PROTOCOL);
                            debug("Invalid response:");
                            debug(cmdRxBuffer);
                        }
                        currentCommand = NULL;
                    }
                    cmdRxBufferPos = 0;
                }
            }
        }
        if (mode == COMMAND) {
            if (currentCommand == NULL) {
                if (commandQueuePos > 0 && !enterDataMode) {
                    // send next command
                    currentCommand = commandQueue[0];
                    for(int i = 1; i < commandQueuePos; i++)
                        commandQueue[i - 1] = commandQueue[i];
                    commandQueuePos--;
                    toUART(currentCommand, strlen(currentCommand));
                } else if (!enterDataMode){
                    enterDataMode = true;
                    prepareDataMode();
                }
            }
        }
        return parsed;
    }
    
    int RN52driver::queueCommand(const char *cmd) {
        if (commandQueuePos == CMD_QUEUE_SIZE) {
            onError(5, OVERFLOW);
            return -1;
        }
        
        commandQueue[commandQueuePos++] = cmd;
        if (mode == COMMAND)// || enterCommandMode)
            return 0;
        
        prepareCommandMode();
        return 0;
    }
    
    void RN52driver::parseQResponse(const char data[4]) {
        int profile =
        (getVal(data[0]) << 4 | getVal(data[1])) & 0x0f;
        int state =
        (getVal(data[2]) << 4 | getVal(data[3])) & 0x0f;
        
        //profIAPConnected = profile & 0x01;
        bool lastSppConnected = sppConnected;
        bool lastA2dpConnected = a2dpConnected;
        
        sppConnected = profile & 0x02;
        a2dpConnected = profile & 0x04;
        //profHFPHSPConnected = profile & 0x08;
        
        bool changed = (this->state != state) || (this->profile != profile);
        //bool profilesChanged = this->profile != profile;
        bool streamingChanged = (this->state == 13 && state != 13) || (this->state != 3 && state == 13);
        this->state = state;
        this->profile = profile;
        
        if (changed)
            onStateChange(state, profile);
        
        if (streamingChanged)
            onStreaming(state == 13);
        if (lastSppConnected != sppConnected)
            onProfileChange(SPP, sppConnected);
        if (lastA2dpConnected != a2dpConnected)
            onProfileChange(A2DP, a2dpConnected);
    }
    
    void RN52driver::prepareCommandMode() {
        if (mode == COMMAND)
            return;
        enterCommandMode = true;
        setMode(COMMAND);
        // Serial.println("DEBUG: RN52 'SPP -> CMD'.");
    }
    
    void RN52driver::prepareDataMode() {
        if (mode == DATA)
            return;
        
        if (enterCommandMode) {
            // command mode was attempted but never reached
            enterCommandMode = false;
        }
        setMode(DATA);
        // Serial.println("DEBUG: RN52 'CMD -> SPP'.");
    }
    
    int RN52driver::sendAVCRP(AVCRP cmd)
    {
        if(!a2dpConnected) {
            onError(6, NOTCONNECTED);
            // Serial.println("ERROR: RN52 A2DP not connected.");
            return -2;
        }
        switch(cmd) {
            case PLAYPAUSE:
                queueCommand(RN52_CMD_AVCRP_PLAYPAUSE);
                // Serial.println("DEBUG: Sending 'Play/Pause' command to RN52.");
                break;
            case PREV:
                queueCommand(RN52_CMD_AVCRP_PREV);
                // Serial.println("DEBUG: Sending 'Previous Track' command to RN52.");
                break;
            case NEXT:
                queueCommand(RN52_CMD_AVCRP_NEXT);
                // Serial.println("DEBUG: Sending 'Next Track' command to RN52.");
                break;
            case VASSISTANT:
                queueCommand(RN52_CMD_AVCRP_VASSISTANT);
                // Serial.println("DEBUG: Sending 'Invoke voice assistant' command to RN52.");
                break;
            case VOLUP:
                queueCommand(RN52_CMD_AVCRP_VOLUP);
                break;
            case VOLDOWN:
                queueCommand(RN52_CMD_AVCRP_VOLDOWN);
                break;
            case MAXVOL:
                queueCommand(RN52_CMD_MAXVOL);
                break;
            case PLAY:
                //nice but not reliable:
                if (state != 13)
                    queueCommand(RN52_CMD_AVCRP_PLAYPAUSE);
                break;
            case PAUSE:
                //nice but not reliable:
                if (state == 13)
                    queueCommand(RN52_CMD_AVCRP_PLAYPAUSE);
                break;
        }
        return 0;
    }
    
    void RN52driver::reconnectLast(){
        queueCommand(RN52_CMD_RECONNECTLAST);
        // Serial.println("DEBUG: RN52 connecting to the last known device.");
    }
    void RN52driver::disconnect(){
        queueCommand(RN52_CMD_DISCONNECT);
        // Serial.println("DEBUG: RN52 disconnecting from the 'active' device.");
    }
    void RN52driver::visible(bool visible){
        if (visible) {
            queueCommand(RN52_CMD_DISCOVERY_ON);
            // Serial.println("DEBUG: RN52 discoverable = ON.");
        }
        else {
            queueCommand(RN52_CMD_DISCOVERY_OFF);
            // Serial.println("DEBUG: RN52 discoverable = OFF (connectable).");
        }
    }
    
    void RN52driver::refreshState() {
        queueCommand(RN52_CMD_QUERY);
    }
    
    static int getVal(char c)
    {
        if(c >= '0' && c <= '9')
            return (c - '0');
        else
            return (c-'A'+10);
    }
    
} /* namespace RN52 */