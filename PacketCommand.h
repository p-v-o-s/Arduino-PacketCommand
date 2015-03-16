/**
 * PacketCommand - A Wiring/Arduino library to 
 * 
 * Copyright (C) 2015 Craig Versek <cversek@gmail.com>
 * 
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PACKETCOMMAND_H
#define PACKETCOMMAND_H

#if defined(WIRING) && WIRING >= 100
  #include <Wiring.h>
#elif defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif
#include <Stream.h>


#define PACKETCOMMAND_MAXCOMMANDS_DEFAULT 10
#define MAX_TYPE_ID_LEN 4
// Uncomment the next line to run the library in debug mode (verbose messages)
//#define PACKETCOMMAND_DEBUG

/******************************************************************************/
// Status and Error  Codes
typedef enum StatusCode {
  SUCCESS = 0,
  ERROR_EXCEDED_MAX_COMMANDS = -1,
  ERROR_INVALID_PACKET       = -2,
  ERROR_INVALID_TYPE_ID      = -3,
  ERROR_NO_TYPE_ID_MATCH     = -4,
  ERROR_NULL_HANDLER_FUNCTION_POINTER = -5
} PACKETCOMMAND_STATUS;

/******************************************************************************/
// PacketCommand (extends Print) 
// so that callbacks print 
class PacketCommand{
  public:
    // Constructor
    PacketCommand(int maxCommands = PACKETCOMMAND_MAXCOMMANDS_DEFAULT);
    PACKETCOMMAND_STATUS addCommand(const char *type_id,
                                    const char *name, 
                                    void(*function)(PacketCommand));            // Add a command to the processing dictionary.
    PACKETCOMMAND_STATUS setDefaultHandler(void (*function)(PacketCommand));    // A handler to call when no valid command received.
    PACKETCOMMAND_STATUS readBuffer(char *pkt, size_t len);                     // Read packet header and loacte a matching registered handler function
    PACKETCOMMAND_STATUS dispatch();
    //provide method for printing to logging stream
    //size_t write(uint8_t val);

  private:
    // Command/handler info
    struct PacketCommandInfo {
      char type_id[MAX_TYPE_ID_LEN];     //limited size type ID must be respected!
      const char *name;
      void (*function)(PacketCommand);     //handler callback function
    };                                     
    PacketCommandInfo *_commandList;    //array to hold command entries
    PacketCommandInfo _current_command; //command ready to dispatch
    PacketCommandInfo _default_command; //called when a packet's Type ID is not recognized
    int  _commandCount;
    int  _maxCommands;
    //packet data processing info to track
    char   *_pkt_data;
    int     _pkt_index;
    size_t  _pkt_len;

};

#endif //PACKETCOMMAND_H
