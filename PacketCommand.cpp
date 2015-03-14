/**
 * PacketCommand - A Wiring/Arduino library to 
 * 
 * Copyright (C) 2015 Craig Versek (cversek@gmail.com)
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
//#define PACKETCOMMAND_DEBUG
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
    _commandLookup[i].type_id = 0;
    _commandLookup[i].name = "";
    _commandLookup[i].function = NULL;
  }
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void PacketCommand::addCommand(const byte *type_id,
                               const char *name,
                               void (*function)(PacketCommand)) {
  #ifdef PacketCommand_DEBUG
    write("Adding command (");
    write(_commandCount);
    write("): ");
    writeln(command);
  #endif
  if (_commandCount >= _maxCommands){
      #ifdef PacketCommand_DEBUG
      write("Error: maxCommands was exceeded");
      #endif
      return;
  }
  //add to the list
  _commandList[_commandCount].type_id  = type_id;
  _commandList[_commandCount].name     = name;
  _commandList[_commandCount].function = function;
  _commandCount++;
}

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void PacketCommand::setDefaultHandler(void (*function)(const char *, PacketCommand)) {
  //make a new command entry
  struct PacketCommandInfo new_info;
  new_info.command  = command;
  new_info.function = function;
  _commandList[_commandCount] = new_info;
  _defaultHandler = function;
}


/**
 * This checks the characters in the buffer looking for a valid type_id prefix
 * string, which adheres to the following pseudocode rules:
 *      For i from 0 to min(len, MAX_TYPE_ID_LEN) if type_id[i] in range from
 *      0x01 to 0xFE, then accept; else if type_id[i] == 0xFF, then increment i
 *      and repeat rule 1; else return with errorcode ERROR_INVALID_TYPE_ID.
 * Once a valid type_id format is detected it is checked against the registered 
 * command entries, and if a match is found it is saved in the _current_handler
 * private attribute;  otherwise, _current_handler is set to NULL
 */
int PacketCommand::readBuffer(byte *pkt, size_t len) {
  byte *type_id[MAX_TYPE_ID_LEN];
  byte cur_byte;
  int pkt_index = 0;
  int type_id_index = 0;
  //parse out type_id from header
  while(pkt_index < len){
    cur_byte = pkt[pkt_index];
    type_id[type_id_index] = cur_byte;
    if (cur_byte == 0x00){
      #ifdef PacketCommand_DEBUG
      write("Error: invalid 'type_id' detected, cannot contain null (0x00) bytes");
      #endif
      return ERROR_INVALID_TYPE_ID;
    }
    elif (cur_byte != 0xFF){ break;}    //valid type ID
    type_id_index++;
    pkt_index++;
    if (type_id_index >= MAX_TYPE_ID_LEN){
      #ifdef PacketCommand_DEBUG
      write("Error: invalid 'type_id' detected, exceeded maximum length");
      #endif
      return ERROR_INVALID_TYPE_ID;
    }
  }
  if ( cur_byte == 0xFF){
  }
    
    for(int i=0; i <= maxCommands; i++){
       if (_commandLookup[i].type_id == com){};
    }
  }
      void (*handler_function)(PacketCommand) =  _commandLookup[];
    }
    else{
      return ERROR_COMMAND_TYPE_INDEX_OUT_OF_RANGE;
    }
  }
  else{
    return ERROR_EMPTY_PACKET;
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
  size_t bytes_out = 0;
  if (_log != NULL){ 
    bytes_out = _port.write(val);
  }
  return bytes_out;
}
