/**
 * PacketCommand - A Wiring/Arduino library to tokenize and parse commands
 * received over a serial port.
 * 
 * Copyright (C) 2014 Craig Versek (cversek@gmail.com)
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
#include "PacketCommand.h"
//#define PacketCommand_DEBUG
/**
 * Constructor makes sure some things are set.
 */
PacketCommand::PacketCommand(int maxCommands,
                             Stream &log,
                            )
  : _log(log),           // reference must be initialized right away
    _commandList(NULL),
    _commandCount(0),
{
  _maxCommands = maxCommands;
  //allocate memory for the command lookup and intialize with null pointers
  _commandLookup = (PacketCommandInfo*) malloc(maxCommands, sizeof(PacketCommandCallback));
  for(int i=0; i <= maxCommands; i++){
    _commandLookup[i].function = NULL;
  }
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void PacketCommand::addCommand(const char *command, void (*function)(PacketCommand)) {
  #ifdef PacketCommand_DEBUG
    Serial.print("Adding command (");
    Serial.print(_commandCount);
    Serial.print("): ");
    Serial.println(command);
  #endif
  if (_commandCount >= _maxCommands){
      #ifdef PacketCommand_DEBUG
      Serial.print("Error: maxCommands was exceeded");
      #endif
      return;
    }
  //make a new callback
  struct PacketCommandCallback new_callback;
  new_callback.command  = command;
  new_callback.function = function;
  _commandList[_commandCount] = new_callback;
  _commandCount++;
}

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void PacketCommand::setDefaultHandler(void (*function)(const char *, PacketCommand)) {
  _defaultHandler = function;
}


/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void PacketCommand::readSerial() {
  while (_port.available() > 0) {
    char inChar = _port.read();   // Read single available character, there may be more waiting
    #ifdef PacketCommand_DEBUG
      Serial.print(inChar);       // Echo back to serial stream
    #endif

    if (inChar == _term) {        // Check for the terminator (default '\r') meaning end of command
      #ifdef PacketCommand_DEBUG
        Serial.print("Received: ");
        Serial.println(_buffer);
      #endif

      char *command = strtok_r(_buffer, _delim, &_last);   // Search for command at start of buffer
      if (command != NULL) {
        bool matched = false;
        for (int i = 0; i < _commandCount; i++) {
          #ifdef PacketCommand_DEBUG
            Serial.print("Comparing [");
            Serial.print(command);
            Serial.print("] to [");
            Serial.print(_commandList[i].command);
            Serial.println("]");
          #endif

          // Compare the found command against the list of known commands for a match
          if (strcmp(command, _commandList[i].command) == 0) {
            #ifdef PacketCommand_DEBUG
              Serial.print("Matched Command: ");
              Serial.println(command);
            #endif

            // Execute the stored handler function for the command,
            // passing in the "this" current PacketCommand object
            (*_commandList[i].function)(*this);
            matched = true;
            break;
          }
        }
        if (!matched && (_defaultHandler != NULL)) {
          (*_defaultHandler)(command, *this);
        }
      }
      clearBuffer();
    }
    else if (isprint(inChar)) {     // Only printable characters into the buffer
      if (_bufPos < PacketCommand_BUFFER) {
        _buffer[_bufPos++] = inChar;  // Put character into buffer
        _buffer[_bufPos] = '\0';      // Null terminate
      } else {
        #ifdef PacketCommand_DEBUG
          Serial.println("Line buffer is full - increase PacketCommand_BUFFER");
        #endif
      }
    }
  }
}

/*
 * Clear the input buffer.
 */
void PacketCommand::clearBuffer() {
  _buffer[0] = '\0';
  _bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *PacketCommand::next() {
  return strtok_r(NULL, _delim, &_last);
}

/*
 * forward all writes to the encapsulated "port" Stream object
 */
size_t PacketCommand::write(uint8_t val) {
  return _port.write(val);
}
