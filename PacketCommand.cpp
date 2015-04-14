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
PacketCommand::PacketCommand(size_t maxCommands,
                             size_t outputBufferSize
                            )
  : _commandCount(0)
{
  _maxCommands = maxCommands;
  //allocate memory for the command lookup and intialize with all null pointers
  _commandList = (CommandInfo*) calloc(maxCommands, sizeof(CommandInfo));
  //allocate memory for the output buffer
  _outputBufferSize = outputBufferSize;
  _output_data  = (byte*) calloc(outputBufferSize, sizeof(byte));
  _output_index = 0;
  _output_len   = 0;
  _write_callback = NULL;
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * The 'type_id' is specially formatted variable-length byte prefix of the form 
 * [0xFF]*[0x01-0xFE] which is matched against the header of each packet processed
 * by readBuffer, and is used to set up the handler function to deal with the remainder
 * of the command parsing.  The 'name' field is a human-readable string that
 * has no special meaning in the current the implmentation.
 */
PacketCommand::STATUS PacketCommand::addCommand(const byte* type_id,
                                               const char* name,
                                               void (*function)(PacketCommand)) {
  byte cur_byte = 0x00;
  int type_id_len = strlen((char*) type_id);
  struct CommandInfo new_command;
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
    if( new_command.type_id[i] != 0x00 ){
      Serial.print(new_command.type_id[i], HEX);
    }
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
PacketCommand::STATUS PacketCommand::registerDefaultHandler(void (*function)(PacketCommand)) {
  if (function != NULL){
    _default_command.function = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to send a 
 * packet back to the interface which dispatched it
 */
PacketCommand::STATUS PacketCommand::registerWriteCallback(void (*function)(byte* pkt, size_t len)){
  if (function != NULL){
    _write_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

PacketCommand::STATUS PacketCommand::lookupCommandByName(const char* name){
  _current_command = _default_command;
  #ifdef PACKETCOMMAND_DEBUG
  Serial.print(F("Searching for command named = "));
  Serial.println(name);
  #endif
  for(int i=0; i <= _maxCommands; i++){
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("\tsearching command at index="));
    Serial.println(i);
    Serial.print(F("\ttype_id["));Serial.print(_input_index);Serial.print(F("]="));
    Serial.println(_commandList[i].type_id[_input_index]);
    #endif
    if(strcmp(_commandList[i].name,name) == 0){
       //a match has been found, so save it and stop
       #ifdef PACKETCOMMAND_DEBUG
       Serial.println(F("match found"));
       #endif
       _current_command = _commandList[i];
       return SUCCESS;
    }
  }
  return ERROR_NO_COMMAND_NAME_MATCH;
}
/**
 * This checks the characters in the buffer looking for a valid type_id prefix
 * string, which adheres to the following pseudocode rules:
 *      For i from 0 to min(len, MAX_TYPE_ID_LEN) 
 *      (1) if type_id[i] is in the character set [0x01-0xFE], then accept and break out of loop; 
 *      (2) else if type_id[i] == 0xFF, then 
 *           (2a) if i is not at the last value, then repeat the loop with the next value of i; 
 *           (2b) else return with errocode ERROR_INVALID_PACKET
 *      (3) else (must be type_id[i] == 0x00) return with errorcode ERROR_INVALID_TYPE_ID.
 *
 * Once a valid type_id format is detected it is checked against the registered 
 * command entries, and if a match is found it is saved in the _current_command
 * private attribute and we return SUCCESS. If no match is found, then the _current_command.function 
 * is set to _default_command.function as long as that has been setup by a call to setDefaultHandler, 
 * such that it is not NULL; otherwise, we return ERROR_NO_TYPE_ID_MATCH.  If return is SUCCESS, then
 * the packet buffer position will have been moved past the type ID field to prepare for parsing any 
 * following binary fields; otherwise, the packet buffer position will remain at the byte that caused 
 * the error condition.
 */
PacketCommand::STATUS PacketCommand::recv(byte* pkt, size_t len) {
  byte cur_byte = 0x00;
  _current_command = _default_command;
  _input_data  = pkt;
  _input_index = 0;
  _input_len = len;
  //parse out type_id from header
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::readBuffer"));
  #endif
  while(_input_index < (int) _input_len){
    cur_byte = _input_data[_input_index];
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("cur_byte="));Serial.println(cur_byte,HEX);
    #endif
    if (cur_byte != 0xFF and cur_byte != 0x00){ //valid type ID completed
      #ifdef PACKETCOMMAND_DEBUG
      Serial.println(F("valid 'type ID' format detected"));
      #endif
      _current_command.type_id[_input_index] = cur_byte;
      break;
    }  
    else if (cur_byte == 0xFF){ //extended type ID, need to check the next byte
      _current_command.type_id[_input_index] = 0xFF;
      _input_index++;
      if (_input_index >= MAX_TYPE_ID_LEN){
        #ifdef PACKETCOMMAND_DEBUG
        Serial.println(F("Error: invalid 'type ID' detected, exceeded maximum length"));
        #endif
        return ERROR_INVALID_TYPE_ID;
      }
      else if (_input_index >= (int) _input_len ){  //0xFF cannot end the type_id
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
  //For a valid type ID 'cur_byte' will be euqal to its last byte and all previous
  //bytes, if they exist,  must have been 0xFF (or nothing), so we only need to check the 
  //corresponding byte for a match in the registered command list.  Also,
  //since pkt_index must be < MAX_TYPE_ID_LEN at this point, it should be within
  //the bounds; and since 'cur_byte' != 0x00 as well, shorter type IDs should
  //not match since the unused bytes are initialized to 0x00.
  for(int i=0; i <= _maxCommands; i++){
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("Searching command at index="));
    Serial.println(i);
    Serial.print(F("\ttype_id["));Serial.print(_input_index);Serial.print(F("]="));
    Serial.println(_commandList[i].type_id[_input_index]);
    #endif
    if(_commandList[i].type_id[_input_index] == cur_byte){
       //a match has been found, so save it and stop
       #ifdef PACKETCOMMAND_DEBUG
       Serial.println(F("match found"));
       #endif
       _current_command = _commandList[i];
       return moveInputBufferIndex(1);  //increment to prepare for data unpacking
    }
  }
  //no type ID has been matched
  if (_default_command.function != NULL){  //set the default handler if it has been registered
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("Setting the default command handler"));
    #endif
    _current_command.function = _default_command.function;
    return moveInputBufferIndex(1);  //increment to prepare for data unpacking
  }
  else{  //otherwise return and error condition
      #ifdef PACKETCOMMAND_DEBUG
      Serial.println(F("No match found for this packet's type ID"));
      #endif
      return ERROR_NO_TYPE_ID_MATCH;
  }
}

// Use the '_write_callback' to send return packet if register
PacketCommand::STATUS PacketCommand::send(){
  if (_write_callback != NULL){
    (*_write_callback)(_output_data, _output_len);
    return SUCCESS;
  }
  else{
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("Error: tried write using a NULL write callback function pointer"));
  #endif
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * Send back the currently active command info structure
*/
PacketCommand::CommandInfo PacketCommand::getCurrentCommand() {
    return _current_command;
}

/**
 * Execute the stored handler function for the current command,
 * passing in "this" current PacketCommandCommand object
*/
PacketCommand::STATUS PacketCommand::dispatch() {
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

/**
 * Accessors and mutators for the input buffer
*/
int PacketCommand::getInputBufferIndex(){
  return _input_index;
}

PacketCommand::STATUS PacketCommand::setInputBufferIndex(int new_index){
  if (new_index >= 0 && 
      new_index <  (int) _input_len + 1){
    _input_index = new_index;
    return SUCCESS;
  }
  else{
    return ERROR_PACKET_INDEX_OUT_OF_BOUNDS;
  }
}

PacketCommand::STATUS PacketCommand::moveInputBufferIndex(int n){
  return setInputBufferIndex(_input_index + n);
}


/**
 * Accessors and mutators for the output buffer
*/
int PacketCommand::getOutputBufferIndex(){
  return _output_index;
}

PacketCommand::STATUS PacketCommand::setOutputBufferIndex(int new_index){
  if (new_index >= 0 && 
      new_index <  (int) _outputBufferSize + 1){
    _output_index = new_index;
    if(_output_index > _output_len){ //adjust output len up
      _output_len = _output_index;
    }
    return SUCCESS;
  }
  else{
    return ERROR_PACKET_INDEX_OUT_OF_BOUNDS;
  }
}

PacketCommand::STATUS PacketCommand::moveOutputBufferIndex(int n){
  return setOutputBufferIndex(_output_index + n);
}
/******************************************************************************/
// Byte field unpacking methods from input buffer
/******************************************************************************/
//bytes and chars
PacketCommand::STATUS PacketCommand::unpack_byte(byte& varByRef){
  varByRef = *((byte*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(byte));
}

PacketCommand::STATUS PacketCommand::unpack_byte_array(byte* buffer, int len){
  for(int i=0; i < len; i++){
    buffer[i] = _input_data[_input_index + i];
  }
  return moveInputBufferIndex(len*sizeof(byte)); //FIXME should be len-1?
}

PacketCommand::STATUS PacketCommand::unpack_char(char& varByRef){
  varByRef = *((char*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(char));
}

PacketCommand::STATUS PacketCommand::unpack_char_array(char* buffer, int len){
  for(int i=0; i < len; i++){
    buffer[i] = _input_data[_input_index + i];
  }
  return moveInputBufferIndex(len*sizeof(char)); //FIXME should be len-1?
}

//stdint types
PacketCommand::STATUS PacketCommand::unpack_int8(int8_t& varByRef){
  varByRef = *((int8_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(int8_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint8(uint8_t& varByRef){
  varByRef = *((uint8_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(uint8_t));
}

PacketCommand::STATUS PacketCommand::unpack_int16(int16_t& varByRef){
  varByRef = *((int16_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(int16_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint16(uint16_t& varByRef){
  varByRef = *((uint16_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(uint16_t));
}

PacketCommand::STATUS PacketCommand::unpack_int32(int32_t& varByRef){
  varByRef = *((int32_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(int32_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint32(uint32_t& varByRef){
  varByRef = *((uint32_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(uint32_t));
}

PacketCommand::STATUS PacketCommand::unpack_int64(int64_t& varByRef){
  varByRef = *((int64_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(int64_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint64(uint64_t& varByRef){
  varByRef = *((uint64_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(uint64_t));
}

//floating point

PacketCommand::STATUS PacketCommand::unpack_float(float& varByRef){
  varByRef = *((float*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(float));
}

PacketCommand::STATUS PacketCommand::unpack_double(double& varByRef){
  varByRef = *((double*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(double));
}

PacketCommand::STATUS PacketCommand::unpack_float32(float32_t& varByRef){
  varByRef = *((float32_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(float32_t));
}

PacketCommand::STATUS PacketCommand::unpack_float64(float64_t& varByRef){
  varByRef = *((float64_t*)(_input_data+_input_index));
  return moveInputBufferIndex(sizeof(float64_t));
}

/******************************************************************************/
// Byte field packing methods into output buffer
/******************************************************************************/
PacketCommand::STATUS PacketCommand::setupOutputCommandByName(const char* name){
  STATUS pcs;
  byte cur_byte; 
  pcs = lookupCommandByName(name);  //sets _current_command on SUCCESS
  //reset output buffer state
  _output_index = 0;
  _output_len   = 0;
  if(pcs == SUCCESS){
    for(int i=0;i<MAX_TYPE_ID_LEN;i++){
        cur_byte = _current_command.type_id[i];
        if(cur_byte == 0x00){ break;}
        pack_byte(cur_byte);
    }
    return SUCCESS;
  }
  else{return pcs;}
}

//bytes and chars
PacketCommand::STATUS PacketCommand::pack_byte(byte value){
  memcpy( (_output_data + _output_index), &value, sizeof(byte));
  return moveOutputBufferIndex(sizeof(byte));
}

PacketCommand::STATUS PacketCommand::pack_byte_array(byte* buffer, int len){
  memcpy( (_output_data + _output_index), buffer, len*sizeof(byte)); //FIXME should be len-1?
  return moveOutputBufferIndex(len*sizeof(byte));
}

PacketCommand::STATUS PacketCommand::pack_char(char value){
  memcpy( (_output_data + _output_index), &value, sizeof(char));
  return moveOutputBufferIndex(sizeof(char));
}

PacketCommand::STATUS PacketCommand::pack_char_array(char* buffer, int len){
  memcpy( (_output_data + _output_index), buffer, len*sizeof(char));
  return moveOutputBufferIndex(len*sizeof(char)); //FIXME should be len-1?
}

//stdint types
PacketCommand::STATUS PacketCommand::pack_int8(int8_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(int8_t));
  return moveOutputBufferIndex(sizeof(int8_t));
}

PacketCommand::STATUS PacketCommand::pack_uint8(uint8_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(uint8_t));
  return moveOutputBufferIndex(sizeof(uint8_t));
}

PacketCommand::STATUS PacketCommand::pack_int16(int16_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(int16_t));
  return moveOutputBufferIndex(sizeof(int16_t));
}

PacketCommand::STATUS PacketCommand::pack_uint16(uint16_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(uint16_t));
  return moveOutputBufferIndex(sizeof(uint16_t));
}

PacketCommand::STATUS PacketCommand::pack_int32(int32_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(int32_t));
  return moveOutputBufferIndex(sizeof(int32_t));
}

PacketCommand::STATUS PacketCommand::pack_uint32(uint32_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(uint32_t));
  return moveOutputBufferIndex(sizeof(uint32_t));
}

PacketCommand::STATUS PacketCommand::pack_int64(int64_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(int64_t));
  return moveOutputBufferIndex(sizeof(int64_t));
}

PacketCommand::STATUS PacketCommand::pack_uint64(uint64_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(uint64_t));
  return moveOutputBufferIndex(sizeof(uint64_t));
}

//floating point

PacketCommand::STATUS PacketCommand::pack_float(float value){
  memcpy( (_output_data + _output_index), &value, sizeof(float));
  return moveOutputBufferIndex(sizeof(float));
}

PacketCommand::STATUS PacketCommand::pack_double(double value){
  memcpy( (_output_data + _output_index), &value, sizeof(double));
  return moveOutputBufferIndex(sizeof(double));
}

PacketCommand::STATUS PacketCommand::pack_float32(float32_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(float32_t));
  return moveOutputBufferIndex(sizeof(float32_t));
}

PacketCommand::STATUS PacketCommand::pack_float64(float64_t value){
  memcpy( (_output_data + _output_index), &value, sizeof(float64_t));
  return moveOutputBufferIndex(sizeof(float64_t));
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
