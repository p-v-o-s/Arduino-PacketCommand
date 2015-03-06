/**
 * PacketCommand - A Wiring/Arduino library to tokenize and parse commands
 * received over a serial port.
 * 
 * Copyright (C) 2012 Stefan Rado
 * Copyright (C) 2011 Steven Cogswell <steven.cogswell@gmail.com>
 *                    http://husks.wordpress.com
 * 
 * Version 20120522
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
#ifndef PacketCommand_h
#define PacketCommand_h

#if defined(WIRING) && WIRING >= 100
  #include <Wiring.h>
#elif defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif
#include <Stream.h>


#define PACKETCOMMAND_MAXCOMMANDS_DEFAULT 10

// Uncomment the next line to run the library in debug mode (verbose messages)
//#define PACKETCOMMAND_DEBUG

/******************************************************************************/
// PacketCommand (extends Print) 
// so that callbacks print 
class PacketCommandDispatcher : public Print {
  public:
    // Constructor
    PacketCommand(int maxCommands = PACKETCOMMAND_MAXCOMMANDS_DEFAULT,
                  Stream &log = NULL
                 );       
    void addCommand(const byte *type_id, void(*function)(PacketCommandDispatcher));       // Add a command to the processing dictionary.
    void setDefaultHandler(void (*function)(PacketCommandDispatcher));                    // A handler to call when no valid command received.
    void handle(byte *pkt, size_t len);    // Read packet header and dispatch to the registered handler function
    //provide method for printing to logging stream
    size_t writeLog(uint8_t val);

  private:
    //Stream object for logging output
    Stream &_log;
    // Command/handler info
    struct PacketCommandInfo {
      void (*function)(PacketCommandDispatcher);  //handler callback function
    };                                     
    PacketCommandInfo *_commandLookup;   // Registry for commands, indexed by type_id
    int  _commandCount;
    int  _maxCommands;

//    // Pointer to the default handler function
//    void (*_defaultHandler)(PacketCommandDispatcher);
};

#endif //PacketCommand_h
