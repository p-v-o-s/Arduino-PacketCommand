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
/**
 * Constructor makes sure some things are set.
 */
PacketCommand::PacketCommand(int maxCommands)
  : _commandCount(0)
{
  _maxCommands = maxCommands;
  //allocate memory for the command lookup and intialize with all null pointers
  _commandList = (PacketCommandInfo*) calloc(maxCommands, sizeof(PacketCommandInfo));
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
PACKETCOMMAND_STATUS PacketCommand::addCommand(const char *type_id,
                                               const char *name,
                                               void (*function)(PacketCommand)) {
  byte cur_byte = 0x00;
  int type_id_len = strlen(type_id);
  struct PacketCommandInfo new_command;
  #ifdef PACKETCOMMAND_DEBUG
  Serial.print(F("Adding command #("));
  Serial.print(_commandCount);
  Serial.print(F("): "));
  Serial.println(name);
  Serial.println(type_id_len);
  #endif
  if (_commandCount >= _maxCommands){
      #ifdef PACKETCOMMAND_DEBUG
      Serial.print(F("Error: exceeded maxCommands="));
      Serial.println(_maxCommands);
      #endif
      return ERROR_EXCEDED_MAX_COMMANDS;
  }
  if (type_id_len > MAX_TYPE_ID_LEN){
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("Error: 'type_id' cannot exceed MAX_TYPE_ID_LEN="));
    Serial.println(MAX_TYPE_ID_LEN);
    #endif
    return ERROR_INVALID_TYPE_ID;
  }
  //check the type_id format
  for(int i=0; i < MAX_TYPE_ID_LEN; i++){
    if (i < type_id_len){
      //test if the type ID rules are followed
      cur_byte = type_id[i];
      #ifdef PACKETCOMMAND_DEBUG
      Serial.print(F("checking type ID byte: "));
      Serial.println(cur_byte, HEX);
      #endif
      switch(cur_byte){
        case 0xFF:
          if (i < (type_id_len - 1)){
            //continue extended type ID
            #ifdef PACKETCOMMAND_DEBUG
            Serial.println(F("\tcontinue extended type ID"));
            #endif
            new_command.type_id[i] = 0xFF;
          }
          else{//cannot end type_id with 0xFF
            #ifdef PACKETCOMMAND_DEBUG
            Serial.println(F("Error: 'type_id' cannot end with 0xFF"));
            #endif
            return ERROR_INVALID_TYPE_ID;
          }
          break;
        case 0x00:
          #ifdef PACKETCOMMAND_DEBUG
          Serial.println(F("Error: 'type_id' cannot contain null (0x00) bytes"));
          #endif
          return ERROR_INVALID_TYPE_ID;
          break;
        default:  //any other byte value
          if(i == (type_id_len - 1)){//valid type ID completed
            #ifdef PACKETCOMMAND_DEBUG
            Serial.println(F("valid type ID completed"));
            #endif
            new_command.type_id[i] = cur_byte;
          }
          else{
            #ifdef PACKETCOMMAND_DEBUG
            Serial.println(F("Error: 'type_id' cannot have a prefix != [0xFF]*"));
            #endif
            return ERROR_INVALID_TYPE_ID;
          }
      }
      //end testing bytes of type_id
    }
    else{  //pad remainder with zeros
      new_command.type_id[i] = 0x00;
    }
  }
  #ifdef PACKETCOMMAND_DEBUG
  Serial.print(F("type_id="));
  for(int i=0; i < MAX_TYPE_ID_LEN; i++){
    Serial.print(new_command.type_id[i], HEX);
  }
  Serial.println();
  #endif
  //finish formatting command info
  new_command.name     = name;
  new_command.function = function;
  _commandList[_commandCount] = new_command;
  _commandCount++;
  return SUCCESS;
}

/**
 * This sets up a handler to be called in the event that packet is send for
 * which a type ID cannot be matched
 */
PACKETCOMMAND_STATUS PacketCommand::setDefaultHandler(void (*function)(PacketCommand)) {
  //
  if (function != NULL){
    _default_command.function = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
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
PACKETCOMMAND_STATUS PacketCommand::readBuffer(char *pkt, size_t len) {
  char cur_byte = 0x00;
  _current_command = _default_command;
  _pkt_data  = pkt;
  _pkt_index = 0;
  _pkt_len = len;
  //parse out type_id from header
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::readBuffer"));
  #endif
  while(_pkt_index < (int) _pkt_len){
    cur_byte = _pkt_data[_pkt_index];
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("cur_byte="));Serial.println(cur_byte,HEX);
    #endif
    if (cur_byte != 0xFF and cur_byte != 0x00){ //valid type ID completed
      #ifdef PACKETCOMMAND_DEBUG
      Serial.println(F("valid 'type ID' format detected"));
      #endif
      break;
    }  
    else if (cur_byte == 0xFF){ //extended type ID, need to check the next byte
      _pkt_index++;
      if (_pkt_index >= MAX_TYPE_ID_LEN){
        #ifdef PACKETCOMMAND_DEBUG
        Serial.println(F("Error: invalid 'type ID' detected, exceeded maximum length"));
        #endif
        return ERROR_INVALID_TYPE_ID;
      }
      else if (_pkt_index >= (int) _pkt_len ){  //0xFF cannot end the type_id
        #ifdef PACKETCOMMAND_DEBUG
        Serial.println(F("Error: invalid packet detected, 'type ID' does not terminate before reaching end of packet"));
        #endif
        return ERROR_INVALID_PACKET;
      }
    }
    else{ //must be 0x00
      #ifdef PACKETCOMMAND_DEBUG
      Serial.println(F("Error: invalid 'type ID' detected, cannot contain null (0x00) bytes"));
      #endif
      return ERROR_INVALID_TYPE_ID;
    }
  }
  //For a valid type ID 'cur_byte' will be euqal its last byte and all previous
  //bytes must have been 0xFF or nothing, so we only need to check the 
  //corresponding byte for a match in the registered command list.  Also,
  //since pkt_index must be < MAX_TYPE_ID_LEN at this point, it should be within
  //the bounds; and since 'cur_byte' != 0x00 as well, shorter type IDs should
  //not match since the unused bytes are initialized to 0x00.
  for(int i=0; i <= _maxCommands; i++){
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("Searching command at index="));
    Serial.println(i);
    Serial.print(F("\ttype_id["));Serial.print(_pkt_index);Serial.print(F("]="));
    Serial.println(_commandList[i].type_id[_pkt_index]);
    #endif
    if(_commandList[i].type_id[_pkt_index] == cur_byte){
       //a match has been found, so save it and stop
       #ifdef PACKETCOMMAND_DEBUG
       Serial.println(F("match found"));
       #endif
       _current_command = _commandList[i];
       return SUCCESS;
    }
  }
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("No match found for this packet's type ID"));
  #endif
  return ERROR_NO_TYPE_ID_MATCH;
}

/**
 * Execute the stored handler function for the current command,
 * passing in the "this" current PacketCommandCommand object
*/
PACKETCOMMAND_STATUS PacketCommand::dispatch() {
  if (_current_command.function != NULL){
    (*_current_command.function)(*this);
    return SUCCESS;
  }
  else{
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("Error: tried to dispatch a NULL handler function pointer"));
  #endif
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

int PacketCommand::getPacketIndex(){
  return _pkt_index;
}

PACKETCOMMAND_STATUS PacketCommand::setPacketIndex(int new_pkt_index){
  if (new_pkt_index >= 0 && 
      new_pkt_index <  (int) _pkt_len){
    _pkt_index = new_pkt_index;
    return SUCCESS;
  }
  else{
    return ERROR_PACKET_INDEX_OUT_OF_BOUNDS;
  }
}

PACKETCOMMAND_STATUS PacketCommand::movePacketIndex(int n){
  return setPacketIndex(_pkt_index + n);
}

///**
// * Retrieve the next token ("word" or "argument") from the command buffer.
// * Returns NULL if no more tokens exist.
// */
//char *PacketCommand::next() {
//  return strtok_r(NULL, _delim, &_last);
//}

/*
 * forward all writes to the encapsulated "log" Stream object
 */
//size_t PacketCommand::write(uint8_t val) {
//  size_t bytes_out = 0;
//  if (_log != NULL){ 
//    bytes_out = _log.write(val);
//  }
//  return bytes_out;
//}
