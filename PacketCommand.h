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
#define ERROR_EMPTY_PACKET -1
#define ERROR_COMMAND_TYPE_INDEX_OUT_OF_RANGE -2
#define ERROR_INVALID_TYPE_ID -3
// Uncomment the next line to run the library in debug mode (verbose messages)
//#define PACKETCOMMAND_DEBUG

/******************************************************************************/
// PacketCommand (extends Print) 
// so that callbacks print 
class PacketCommand : public Print {
  public:
    // Constructor
    PacketCommand(int maxCommands = PACKETCOMMAND_MAXCOMMANDS_DEFAULT,
                  Stream &log = NULL
                 );       
    void addCommand(const byte *type_id, void(*function)(PacketCommand));       // Add a command to the processing dictionary.
    void setDefaultHandler(void (*function)(PacketCommand));                    // A handler to call when no valid command received.
    void readBuffer(byte *pkt, size_t len);    // Read packet header and dispatch to the registered handler function
    void dispatch();
    //provide method for printing to logging stream
    size_t write(uint8_t val);

  private:
    //Stream object for logging output
    Stream &_log;
    // Command/handler info
    struct PacketCommandInfo {
      const byte *type_id;
      const char *name;
      void (*function)(PacketCommand);  //handler callback function
    };                                     
    PacketCommandInfo *_commandList;    //array to hold command entries
    PacketCommandInfo _current_command ; //
    int  _commandCount;
    int  _maxCommands;

//    // Pointer to the default handler function
//    void (*_defaultHandler)(PacketCommandDispatcher);
};

#endif //PACKETCOMMAND_H
