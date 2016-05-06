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
 *  Created on: Jun 21, 2013
 *  Modified by: Karlis Veilands on May, 2016
 */

#ifndef RN52STRINGS_H_
#define RN52STRINGS_H_

const char *RN52_CMD_BEGIN = "CMD\r\n";
const char *RN52_CMD_EXIT = "END\r\n";
const char *RN52_CMD_QUERY = "Q\r";
const char *RN52_CMD_DETAILS = "D\r";
const char *RN52_CMD_RECONNECTLAST = "B,04\r";
const char *RN52_CMD_DISCONNECT = "K,06\r";

const char* RN52_CMD_RESET = "R,1\r";
const char* RN52_CMD_AVCRP_VOLUP = "AV+\r";
const char* RN52_CMD_AVCRP_VOLDOWN = "AV-\r";
const char* RN52_CMD_AVCRP_NEXT = "AT+\r";
const char* RN52_CMD_AVCRP_PREV = "AT-\r";
const char* RN52_CMD_AVCRP_PLAYPAUSE = "AP\r";
const char* RN52_CMD_DISCOVERY_ON = "Q,1\r";
const char* RN52_CMD_DISCOVERY_OFF = "Q,0\r";

const char *RN52_RX_OK = "AOK\r\n";
const char *RN52_RX_ERROR = "ERR\r\n";
const char *RN52_RX_WHAT = "?\r\n";


#endif /* RN52STRINGS_H_ */
