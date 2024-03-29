/**
 * PacketCommand - An Arduino library to 
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

#include <Arduino.h>

/**
 * Constructor makes sure some things are set.
 */
PacketCommand::PacketCommand(size_t maxCommands,
                             size_t inputBufferSize,
                             size_t outputBufferSize
                            ){
  _maxCommands = maxCommands;
  //allocate memory for the command lookup and intialize with all null pointers
  _commandList = (CommandInfo*) calloc(maxCommands, sizeof(CommandInfo));
  //allocate memory for the input buffer
  _inputBufferSize = inputBufferSize;
  if (_inputBufferSize > 0){ //allocate memory
     allocateInputBuffer(inputBufferSize);
  }
  else{ //do not allocate anything
    _input_buffer = nullptr;
  }
  //allocate memory for the output buffer
  _outputBufferSize = outputBufferSize;
  if (  _outputBufferSize > 0){ //allocate memory
     allocateOutputBuffer(outputBufferSize);
  }
  else{ //do not allocate anything
    _output_buffer = nullptr;
  }
  reset();
}

/**
 * Resets the state of the command handler, undoing any addCommand calls,
 * callback registrations, and buffer state changes.
 */
PacketShared::STATUS PacketCommand::reset(){
  //wipe the command list clean
  for(size_t i=0; i < _commandCount; i++){
    for(size_t j=0; j < MAX_TYPE_ID_LEN; j++){
      _commandList[i].type_id[j] = 0x00;
    }
    _commandList[i].name = "";
    _commandList[i].function = nullptr;
  }
  _commandCount = 0;
  //reset input buffer
  _input_index = 0;
  _input_len   = 0;
  _input_flags = 0x00;
  _input_properties.from_addr = 0;
  _input_properties.RSSI      = 0;
  _recv_timestamp_micros = 0;
  //reset output buffer
  _output_index = 0;
  _output_len   = 0;
  _output_flags = 0x00;
  _output_to_address = 0;
  //null out unregistered callbacks
  _recv_callback = nullptr;
  _reply_send_callback = nullptr;
  _send_callback = nullptr;
  _send_nonblocking_callback = nullptr;
  _reply_recv_callback = nullptr;
  return PacketShared::SUCCESS;
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * The 'type_id' is specially formatted variable-length byte prefix of the form 
 * [0xFF]*[0x01-0xFE] which is matched against the header of each packet processed
 * by readBuffer, and is used to set up the handler function to deal with the remainder
 * of the command parsing.  The 'name' field is a human-readable string that
 * has no special meaning in the current the implmentation.
 */
PacketShared::STATUS PacketCommand::addCommand(const byte* type_id,
                                                const char* name,
                                                void (*function)(PacketCommand&)) {
  byte cur_byte = 0x00;
  size_t type_id_len = strlen((char*) type_id);
  struct CommandInfo new_command;
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# in PacketCommand::addCommand"));
  PACKETCOMMAND_DEBUG_PORT.print(F("#\tAdding command #("));
  PACKETCOMMAND_DEBUG_PORT.print(_commandCount);
  PACKETCOMMAND_DEBUG_PORT.print(F("): "));
  PACKETCOMMAND_DEBUG_PORT.println(name);
  PACKETCOMMAND_DEBUG_PORT.print("'");
  PACKETCOMMAND_DEBUG_PORT.print((char *) type_id);
  PACKETCOMMAND_DEBUG_PORT.println("'");
  PACKETCOMMAND_DEBUG_PORT.println(type_id_len);
  #endif
  if (_commandCount >= _maxCommands - 1){
      #ifdef PACKETCOMMAND_DEBUG
      PACKETCOMMAND_DEBUG_PORT.print(F("### Error: exceeded maxCommands="));
      PACKETCOMMAND_DEBUG_PORT.println(_maxCommands);
      #endif
      return PacketShared::ERROR_EXCEDED_MAX_COMMANDS;
  }
  if (type_id_len > MAX_TYPE_ID_LEN){
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.print(F("### Error: 'type_id' cannot exceed MAX_TYPE_ID_LEN="));
    PACKETCOMMAND_DEBUG_PORT.println(MAX_TYPE_ID_LEN);
    #endif
    return PacketShared::ERROR_INVALID_TYPE_ID;
  }
  //check the type_id format
  for(size_t i=0; i < MAX_TYPE_ID_LEN; i++){
    if (i < type_id_len){
      //test if the type ID rules are followed
      cur_byte = type_id[i];
      #ifdef PACKETCOMMAND_DEBUG
      PACKETCOMMAND_DEBUG_PORT.print(F("#\ti="));
      PACKETCOMMAND_DEBUG_PORT.println(i);
      PACKETCOMMAND_DEBUG_PORT.print(F("#\tchecking type ID byte: "));
      PACKETCOMMAND_DEBUG_PORT.println(cur_byte, HEX);
      #endif
      switch(cur_byte){
        case 0xFF:
          if (i < (type_id_len - 1)){
            //continue extended type ID
            #ifdef PACKETCOMMAND_DEBUG
            PACKETCOMMAND_DEBUG_PORT.println(F("#\tcontinue extended type ID"));
            #endif
            new_command.type_id[i] = 0xFF;
          }
          else{//cannot end type_id with 0xFF
            #ifdef PACKETCOMMAND_DEBUG
            PACKETCOMMAND_DEBUG_PORT.println(F("### Error: 'type_id' cannot end with 0xFF"));
            #endif
            return PacketShared::ERROR_INVALID_TYPE_ID;
          }
          break;
        case 0x00:
          #ifdef PACKETCOMMAND_DEBUG
          PACKETCOMMAND_DEBUG_PORT.println(F("### Error: 'type_id' cannot contain null (0x00) bytes"));
          #endif
          return PacketShared::ERROR_INVALID_TYPE_ID;
          break;
        default:  //any other byte value
          if(i == (type_id_len - 1)){//valid type ID completed
            #ifdef PACKETCOMMAND_DEBUG
            PACKETCOMMAND_DEBUG_PORT.println(F("#\tvalid type ID completed"));
            #endif
            new_command.type_id[i] = cur_byte;
          }
          else{
            #ifdef PACKETCOMMAND_DEBUG
            PACKETCOMMAND_DEBUG_PORT.println(F("### Error: 'type_id' cannot have a prefix != [0xFF]*"));
            #endif
            return PacketShared::ERROR_INVALID_TYPE_ID;
          }
          break;
      }
      //end testing bytes of type_id
    }
    else{  //pad remainder with zeros
      new_command.type_id[i] = 0x00;
    }
  }
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.print(F("#\ttype_id="));
  for(size_t i=0; i < MAX_TYPE_ID_LEN; i++){
    if( new_command.type_id[i] != 0x00 ){
      PACKETCOMMAND_DEBUG_PORT.print(new_command.type_id[i], HEX);
    }
  }
  PACKETCOMMAND_DEBUG_PORT.println();
  #endif
  //finish formatting command info
  new_command.name     = name;
  new_command.function = function;
  _commandList[_commandCount] = new_command;
  _commandCount++;
  return PacketShared::SUCCESS;
}

/**
 * This sets up a handler to be called in the event that packet is send for
 * which a type ID cannot be matched
 */
PacketShared::STATUS PacketCommand::registerDefaultHandler(void (*function)(PacketCommand&)) {
  if (function != nullptr){
    _default_command.function = function;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}


/**
 * This sets up a callback which can be used by a command handler to read a 
 * packet into its input buffer
 */
PacketShared::STATUS PacketCommand::registerRecvCallback(bool (*function)(PacketCommand&)){
  if (function != nullptr){
    _recv_callback = function;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to send a 
 * packet from its output buffer
 */
PacketShared::STATUS PacketCommand::registerReplySendCallback(void (*function)(PacketCommand&)){
  if (function != nullptr){
    _reply_send_callback = function;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to send a 
 * packet from its output buffer
 */
PacketShared::STATUS PacketCommand::registerSendCallback(bool (*function)(PacketCommand&)){
  if (function != nullptr){
    _send_callback = function;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to send a 
 * packet from its output buffer
 */
PacketShared::STATUS PacketCommand::registerSendNonblockingCallback(void (*function)(PacketCommand&)){
  if (function != nullptr){
    _send_nonblocking_callback = function;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to send a 
 * packet from its output buffer
 */
PacketShared::STATUS PacketCommand::registerSendBufferedCallback(void (*function)(PacketCommand&)){
  if (function != nullptr){
    _send_buffered_callback = function;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * 
 */
PacketShared::STATUS PacketCommand::registerReplyRecvCallback(bool (*function)(PacketCommand&)){
  if (function != nullptr){
    _reply_recv_callback = function;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * 
 */
PacketShared::STATUS PacketCommand::processInput(){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::processInput"));
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_index="));DEBUG_PORT.println(_input_index);
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_len="));DEBUG_PORT.println(_input_len);
  #endif
  PacketShared::STATUS pcs = matchCommand();
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# (processInput)-after calling matchCommand()"));
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_index="));DEBUG_PORT.println(_input_index);
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_len="));DEBUG_PORT.println(_input_len);
  #endif
  if (pcs == PacketShared::SUCCESS){  //a command was matched
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.print(F("# (processInput)-matched command: "));
    CommandInfo cmd = getCurrentCommand();
    PACKETCOMMAND_DEBUG_PORT.println(cmd.name);
    #endif
  }
  else if (pcs == PacketShared::ERROR_NO_TYPE_ID_MATCH){  //valid ID but no command was matched
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("# (processInput)-no matched command"));
    #endif
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.print(F("### Error: pCmd.matchCommand returned status code: "));
    PACKETCOMMAND_DEBUG_PORT.println(pcs);
    #endif
    return pcs;
  }
  //dispatch to handler or default if no match
  dispatchCommand();
  return PacketShared::SUCCESS;
}


PacketShared::STATUS PacketCommand::lookupCommandByName(const char* name){
  _current_command = _default_command;
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::lookupCommandByName"));
  PACKETCOMMAND_DEBUG_PORT.print(F("#\tSearching for command named = "));
  PACKETCOMMAND_DEBUG_PORT.println(name);
  #endif
  for(size_t i=0; i < _maxCommands; i++){
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.print(F("#\tsearching command at index="));
    PACKETCOMMAND_DEBUG_PORT.println(i);
    PACKETCOMMAND_DEBUG_PORT.print(F("#\t&_commandList[i]="));
    PACKETCOMMAND_DEBUG_PORT.println((int) &_commandList[i], HEX);
    PACKETCOMMAND_DEBUG_PORT.print(F("#\tname="));
    PACKETCOMMAND_DEBUG_PORT.println(_commandList[i].name);
    #endif
    if(strcmp(_commandList[i].name,name) == 0){
       //a match has been found, so save it and stop
       #ifdef PACKETCOMMAND_DEBUG
       PACKETCOMMAND_DEBUG_PORT.println(F("#\tmatch found"));
       #endif
       _current_command = _commandList[i];
       return PacketShared::SUCCESS;
    }
  }
  return PacketShared::ERROR_NO_COMMAND_NAME_MATCH;
}

PacketShared::STATUS PacketCommand::recv() {
  bool gotPacket = false;
  return recv(gotPacket);
}

PacketShared::STATUS PacketCommand::recv(bool& gotPacket) {
  uint32_t timestamp_micros = micros();
  gotPacket = false;
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::recv()"));
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_index="));DEBUG_PORT.println(_input_index);
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_len="));DEBUG_PORT.println(_input_len);
  #endif
  //call the read callback which should load data into _input_buffer and set len
  if (_recv_callback != nullptr){
    gotPacket = (*_recv_callback)(*this);
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried write using a nullptr read callback function pointer"));
    #endif
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
  if (gotPacket){ //we have a packet
    set_recvTimestamp(timestamp_micros);
    return PacketShared::SUCCESS;
  }
  else{  //we have no packet
    return PacketShared::NO_PACKET_RECEIVED;
  }
}

PacketShared::STATUS PacketCommand::set_recvTimestamp(uint32_t timestamp_micros){
  _recv_timestamp_micros = timestamp_micros;
  return PacketShared::SUCCESS;
}

/**
 *g This checks the characters in the buffer looking for a valid type_id prefix
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
 * such that it is not nullptr; otherwise, we return ERROR_NO_TYPE_ID_MATCH.  If return is SUCCESS, then
 * the packet buffer position will have been moved past the type ID field to prepare for parsing any 
 * following binary fields; otherwise, the packet buffer position will remain at the byte that caused 
 * the error condition.
 */
PacketShared::STATUS PacketCommand::matchCommand(){
  byte cur_byte = 0x00;
  _current_command = _default_command;
  int type_id_index = 0;
  //parse out type_id from header
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::matchCommand"));
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_index="));DEBUG_PORT.println(_input_index);
  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_len="));DEBUG_PORT.println(_input_len);
  #endif
  if (_input_index >= _input_len){
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: invalid 'type ID' detected, no packet data left"));
    #endif
    return PacketShared::ERROR_INVALID_TYPE_ID;
  }
  while(_input_index < _input_len){
    cur_byte = _input_buffer[_input_index];
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.print(F("#\tcur_byte="));DEBUG_PORT.println(cur_byte,HEX);
    PACKETCOMMAND_DEBUG_PORT.print(F("#\t_type_id_index="));DEBUG_PORT.println(type_id_index);
    #endif
    if (cur_byte != 0xFF and cur_byte != 0x00){ //valid type ID completed
      #ifdef PACKETCOMMAND_DEBUG
      PACKETCOMMAND_DEBUG_PORT.println(F("#\tvalid 'type ID' format detected"));
      #endif
      _current_command.type_id[type_id_index] = cur_byte;
      //CAUTION do not increment type_id_index or _input_index here!
      break;
    }  
    else if (cur_byte == 0xFF){ //extended type ID, need to check the next byte
      _current_command.type_id[type_id_index] = 0xFF;
      type_id_index++;
      _input_index++;
      if (type_id_index >= MAX_TYPE_ID_LEN){
        #ifdef PACKETCOMMAND_DEBUG
        PACKETCOMMAND_DEBUG_PORT.println(F("### Error: invalid 'type ID' detected, exceeded maximum length"));
        #endif
        return PacketShared::ERROR_INVALID_TYPE_ID;
      }
      else if (_input_index >= _input_len ){  //0xFF cannot end the type_id
        #ifdef PACKETCOMMAND_DEBUG
        PACKETCOMMAND_DEBUG_PORT.println(F("### Error: invalid packet detected, 'type ID' does not terminate before reaching end of packet"));
        #endif
        return PacketShared::ERROR_INVALID_PACKET;
      }
    }
    else{ //must be 0x00
      #ifdef PACKETCOMMAND_DEBUG
      PACKETCOMMAND_DEBUG_PORT.println(F("### Error: invalid 'type ID' detected, cannot contain null (0x00) bytes"));
      #endif
      return PacketShared::ERROR_INVALID_TYPE_ID;
    }
  }
  //For a valid type ID 'cur_byte' will be equal to its last byte and all previous
  //bytes, if they exist,  must have been 0xFF (or nothing), so we only need to check the 
  //corresponding byte for a match in the registered command list.  Also,
  //since pkt_index must be < MAX_TYPE_ID_LEN at this point, it should be within
  //the bounds; and since 'cur_byte' != 0x00 as well, shorter type IDs should
  //not match since the unused bytes are initialized to 0x00.
  for(size_t i=0; i < _maxCommands; i++){
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.print(F("# Searching command at index="));
    PACKETCOMMAND_DEBUG_PORT.println(i);
    PACKETCOMMAND_DEBUG_PORT.print(F("#\ttype_id["));DEBUG_PORT.print(type_id_index);DEBUG_PORT.print(F("]="));
    PACKETCOMMAND_DEBUG_PORT.println(_commandList[i].type_id[type_id_index]);
    #endif
    if(_commandList[i].type_id[type_id_index] == cur_byte){
       //a match has been found, so save it and stop
       #ifdef PACKETCOMMAND_DEBUG
       PACKETCOMMAND_DEBUG_PORT.println(F("#match found"));
       #endif
       _current_command = _commandList[i];
       return moveInputBufferIndex(1);  //increment to prepare for data unpacking
    }
  }
  //no type ID has been matched
  if (_default_command.function != nullptr){  //set the default handler if it has been registered
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("# Setting the default command handler"));
    #endif
    _current_command.function = _default_command.function;
    return moveInputBufferIndex(1);  //increment to prepare for data unpacking
  }
  else{  //otherwise return and error condition
      #ifdef PACKETCOMMAND_DEBUG
      PACKETCOMMAND_DEBUG_PORT.println(F("# No match found for this packet's type ID"));
      #endif
      return PacketShared::ERROR_NO_TYPE_ID_MATCH;
  }
}

/**
 * Send back the currently active command info structure
*/
PacketCommand::CommandInfo PacketCommand::getCurrentCommand() {
  return _current_command;
}

///**
// * Set the currently active command info structure
//*/
//void PacketCommand::setCurrentCommand(PacketCommand::CommandInfo command) {
//  _current_command = command;
//}

/**
 * Execute the stored handler function for the current command,
 * passing in "this" current PacketCommandCommand object
*/
PacketShared::STATUS PacketCommand::dispatchCommand() {
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::dispatchCommand"));
  #endif
  if (_current_command.function != nullptr){
    (*_current_command.function)(*this);
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried to dispatch a nullptr handler function pointer"));
    #endif
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

//use unpack* methods to pull out data from packet

PacketShared::STATUS PacketCommand::setupOutputCommandByName(const char* name){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::setupOutputCommandByName"));
  #endif
  PacketShared::STATUS pcs;
  pcs = lookupCommandByName(name);  //sets _current_command on SUCCESS
  //reset output buffer state
  if(pcs == PacketShared::SUCCESS){
    pcs = setupOutputCommand(_current_command);
    return pcs;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.print(F("### Error: lookupCommandByName failed with errocode: "));
    PACKETCOMMAND_DEBUG_PORT.println(pcs);
    #endif
    return pcs;
  }
}

PacketShared::STATUS PacketCommand::setupOutputCommand(PacketCommand::CommandInfo command){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::setupOutputCommand"));
  #endif
  byte cur_byte;
  for(size_t i=0;i<MAX_TYPE_ID_LEN;i++){
      cur_byte = command.type_id[i];
      if(cur_byte == 0x00){ break;}
      pack_byte(cur_byte);
  }
  return PacketShared::SUCCESS;
}

//use pack* methods to add additional arguments to output buffer

// Use the '_send_callback' to send return packet
PacketShared::STATUS PacketCommand::send(){
  bool sentPacket = false;
  return send(sentPacket);
}

// Use the '_send_callback' to send return packet
PacketShared::STATUS PacketCommand::send(bool& sentPacket){
  uint32_t timestamp_micros = micros();
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::send"));
  PACKETCOMMAND_DEBUG_PORT.print(F("# \ttimestamp_micros: "));
  PACKETCOMMAND_DEBUG_PORT.println(timestamp_micros);
  #endif
  if (_send_callback != nullptr){
//    set_sendTimestamp(timestamp_micros);  //markdown the time write now
//    if (_output_flags & PacketShared::OPFLAG_APPEND_SEND_TIMESTAMP){
//      if ((_output_len + sizeof(uint32_t)) < PacketShared::DATA_BUFFER_SIZE){ //prevent buffer overrun
//        pack_uint32(timestamp_micros);
//      }
//      else{
//        #ifdef PACKETCOMMAND_DEBUG
//        PACKETCOMMAND_DEBUG_PORT.println(F("### Error: appending send timestamp would overrun the output buffer"));
//        #endif
//      }
//    }
    //call the callback!
    sentPacket = (*_send_callback)(*this);
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried to send using a nullptr send callback function pointer"));
    #endif
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

// Use the '_send_nonblocking_callback' to send return packet
PacketShared::STATUS PacketCommand::send_nonblocking(){
  uint32_t timestamp_micros = micros();
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::send_nonblocking"));
  PACKETCOMMAND_DEBUG_PORT.print(F("# \ttimestamp_micros: "));
  PACKETCOMMAND_DEBUG_PORT.println(timestamp_micros);
  #endif
  if (_send_nonblocking_callback != nullptr){
//    set_sendTimestamp(timestamp_micros);  //markdown the time write now
//    if (_output_flags & PacketShared::OPFLAG_APPEND_SEND_TIMESTAMP){
//      if ((_output_len + sizeof(uint32_t)) < PacketShared::DATA_BUFFER_SIZE){ //prevent buffer overrun
//        pack_uint32(timestamp_micros);
//      }
//      else{
//        #ifdef PACKETCOMMAND_DEBUG
//        PACKETCOMMAND_DEBUG_PORT.println(F("### Error: appending send timestamp would overrun the output buffer"));
//        #endif
//      }
//    }
    //call the nonblocking send callback
    (*_send_nonblocking_callback)(*this);
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried to send using a nullptr send nonblocking callback function pointer"));
    #endif
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

// Use the '_send_buffered_callback' to send return packet
PacketShared::STATUS PacketCommand::send_buffered(){
  uint32_t timestamp_micros = micros();
  if (_send_buffered_callback != nullptr){
//    set_sendTimestamp(timestamp_micros);  //markdown the time write now
//    if (_output_flags & PacketShared::OPFLAG_APPEND_SEND_TIMESTAMP){
//      if ((_output_len + sizeof(uint32_t)) < PacketShared::DATA_BUFFER_SIZE){ //prevent buffer overrun
//        pack_uint32(timestamp_micros);
//      }
//      else{
//        #ifdef PACKETCOMMAND_DEBUG
//        PACKETCOMMAND_DEBUG_PORT.println(F("### Error: appending send timestamp would overrun the output buffer"));
//        #endif
//      }
//    }
    //call the nonblocking send callback
    (*_send_buffered_callback)(*this);
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried to send using a nullptr send_buffered_callback function pointer"));
    #endif
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

PacketShared::STATUS PacketCommand::set_sendTimestamp(uint32_t timestamp_micros){
  _send_timestamp_micros = timestamp_micros;
  return PacketShared::SUCCESS;
}

// Use the '_reply_send_callback' to send a quick reply
PacketShared::STATUS PacketCommand::reply_send(){
  if (_reply_send_callback != nullptr){
    //call the send callback
    (*_reply_send_callback)(*this);
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried to send using a nullptr send callback function pointer"));
    #endif
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}


// Use the '_reply_send_callback' to send a quick reply
PacketShared::STATUS PacketCommand::reply_recv(){
  if (_reply_recv_callback != nullptr){
    //call the send callback
    (*_reply_recv_callback)(*this);
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried to receive using a nullptr recv callback function pointer"));
    #endif
    return PacketShared::ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * Accessors and mutators for the input buffer
*/

PacketShared::STATUS PacketCommand::assignInputBuffer(byte* buff, size_t len){
  #ifdef PACKETCOMMAND_DEBUG
//  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::assignInputBuffer"));
//  PACKETCOMMAND_DEBUG_PORT.print(F("#\tlen="));DEBUG_PORT.println(len);
//  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_inputBufferSize="));DEBUG_PORT.println(_inputBufferSize);
//  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_index="));DEBUG_PORT.println(_input_index);
//  PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_len="));DEBUG_PORT.println(_input_len);
  #endif
  _input_buffer = buff;
  //check the input length before setting
  if (len <= _inputBufferSize){
    _input_len = len;
    #ifdef PACKETCOMMAND_DEBUG
//    PACKETCOMMAND_DEBUG_PORT.println(F("# (assignInputBuffer) after setting _input_len"));
//    PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_index="));DEBUG_PORT.println(_input_index);
//    PACKETCOMMAND_DEBUG_PORT.print(F("#\t_input_len="));DEBUG_PORT.println(_input_len);
    #endif
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    PACKETCOMMAND_DEBUG_PORT.println(F("### Error: tried to receive data that would overrun input buffer"));
    #endif
    _input_len = _inputBufferSize; //set to safe value
    return PacketShared::ERROR_INPUT_BUFFER_OVERRUN;
  }
}

void  PacketCommand::allocateInputBuffer(size_t len){
  _inputBufferSize = len;
  _input_buffer = (byte*) calloc(_inputBufferSize, sizeof(byte));
}

byte*  PacketCommand::getInputBuffer(){
  return _input_buffer;
}

int PacketCommand::getInputBufferIndex(){
  return _input_index;
}

PacketShared::STATUS PacketCommand::setInputBufferIndex(int new_index){
  if (new_index >= 0 && 
      new_index <  (int) _input_len + 1){
    _input_index = new_index;
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_PACKET_INDEX_OUT_OF_BOUNDS;
  }
}


PacketShared::STATUS PacketCommand::moveInputBufferIndex(int n){
  return setInputBufferIndex(_input_index + n);
}
    
void PacketCommand::resetInputBuffer(){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::resetInputBuffer"));
  #endif
  _input_index = 0;
  _input_len   = 0;
  _input_flags = 0x00;
}

PacketShared::STATUS PacketCommand::enqueueInputBuffer(PacketQueue& pq){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::enqueueInputBuffer"));
  #endif
  //build a packet struct to hold current buffer state
  PacketShared::Packet pkt;
  pkt.length = min((size_t) _input_len, PacketShared::DATA_BUFFER_SIZE);
  pkt.timestamp = _recv_timestamp_micros;  //this should have been recorded as close to the RX time as possible
  pkt.flags  = _input_flags;
  memcpy(pkt.data, _input_buffer, pkt.length);
  PacketShared::STATUS pqs;
  pqs = pq.enqueue(pkt);
  return pqs;
}

PacketShared::STATUS PacketCommand::dequeueInputBuffer(PacketQueue& pq){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::dequeueInputBuffer"));
  #endif
  //build a packet struct to hold current buffer state
  PacketShared::Packet pkt;
  PacketShared::STATUS pqs;
  pqs = pq.dequeue(pkt);
  if(pqs == PacketShared::SUCCESS){
    _input_index = 0;
    _input_len = min(pkt.length, _inputBufferSize);
    _input_flags = pkt.flags;
    _recv_timestamp_micros = pkt.timestamp; //FIXME make sure timestamp is in micros
    memcpy(_input_buffer, pkt.data, _input_len);
    return PacketShared::SUCCESS;
  }
  else{
    //zero out on failure
    _input_index = 0;
    _input_len   = 0;
    _input_flags = 0x00;
    _recv_timestamp_micros = 0;
    return pqs;
  }
}



void  PacketCommand::allocateOutputBuffer(size_t len){
  _outputBufferSize = len;
  _output_buffer = (byte*) calloc(_outputBufferSize, sizeof(byte));
}

/**
 * Accessors and mutators for the output buffer
*/
int PacketCommand::getOutputBufferIndex(){
  return _output_index;
}

PacketShared::STATUS PacketCommand::setOutputBufferIndex(int new_index){
  if (new_index >= 0 && 
      new_index <  (int) _outputBufferSize + 1){
    _output_index = new_index;
    if(_output_index > _output_len){ //adjust output len up
      _output_len = _output_index;
    }
    return PacketShared::SUCCESS;
  }
  else{
    return PacketShared::ERROR_PACKET_INDEX_OUT_OF_BOUNDS;
  }
}

PacketShared::STATUS PacketCommand::moveOutputBufferIndex(int n){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::moveOutputBufferIndex"));
  PACKETCOMMAND_DEBUG_PORT.print(F("# \t_output_index="));
  PACKETCOMMAND_DEBUG_PORT.println(_output_index);
  PACKETCOMMAND_DEBUG_PORT.print(F("# \tn="));
  PACKETCOMMAND_DEBUG_PORT.println(n);
  #endif
  return setOutputBufferIndex(_output_index + n);
}

void PacketCommand::resetOutputBuffer(){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::resetOutputBuffer"));
  #endif
  _output_index = 0;
  _output_len   = 0;
  _output_flags = 0x00;
}

PacketShared::STATUS PacketCommand::enqueueOutputBuffer(PacketQueue& pq){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::enqueueOutputBuffer"));
  #endif
  //build a packet struct to hold current buffer state
  PacketShared::Packet pkt;
  pkt.length = min((size_t) _output_len, PacketShared::DATA_BUFFER_SIZE);
  pkt.timestamp = 0;
  pkt.flags  = _output_flags;
  memcpy(pkt.data, _output_buffer, pkt.length);
  PacketShared::STATUS pqs;
  pqs = pq.enqueue(pkt);
  return pqs;
}

PacketShared::STATUS PacketCommand::dequeueOutputBuffer(PacketQueue& pq){
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::dequeueOutputBuffer"));
  #endif
  //build a packet struct to hold current buffer state
  PacketShared::Packet pkt;
  PacketShared::STATUS pqs;
  pqs = pq.dequeue(pkt);
  if(pqs == PacketShared::SUCCESS){
    _output_len = min(pkt.length, _outputBufferSize);
    _output_index = _output_len;  //IMPORTANT! set output index end of last entry so stuff could be added properly
    _output_flags = pkt.flags;
    memcpy(_output_buffer, pkt.data, _output_len);
    return PacketShared::SUCCESS;
  }
  else{
    //zero out on failure
    _output_index = 0;
    _output_len = 0;
    _output_flags = 0x00;
    return pqs;
  }
}

PacketShared::STATUS PacketCommand::requeueOutputBuffer(PacketQueue& pq){
  //pushes output buffer onto the front of the queue
  #ifdef PACKETCOMMAND_DEBUG
  PACKETCOMMAND_DEBUG_PORT.println(F("# In PacketCommand::requeueOutputBuffer"));
  #endif
  //build a packet struct to hold current buffer state
  PacketShared::Packet pkt;
  pkt.length = min((size_t) _output_len, PacketShared::DATA_BUFFER_SIZE);
  pkt.timestamp = 0;
  pkt.flags  = _output_flags;
  memcpy(pkt.data, _output_buffer, pkt.length);
  PacketShared::STATUS pqs;
  pqs = pq.requeue(pkt);
  return pqs;
}

/******************************************************************************/
// Byte field unpacking methods from input buffer
/******************************************************************************/
//bytes and chars
PacketShared::STATUS PacketCommand::unpack_byte(byte& varByRef){
  varByRef = *((byte*)(_input_buffer+_input_index));
  return moveInputBufferIndex(sizeof(byte));
}

PacketShared::STATUS PacketCommand::unpack_byte_array(byte* buffer, size_t len){
  for(size_t i=0; i < len; i++){
    buffer[i] = _input_buffer[_input_index + i];
  }
  return moveInputBufferIndex(len*sizeof(byte)); //FIXME should be len-1?
}

PacketShared::STATUS PacketCommand::unpack_char(char& varByRef){
  varByRef = *((char*)(_input_buffer+_input_index));
  return moveInputBufferIndex(sizeof(char));
}

PacketShared::STATUS PacketCommand::unpack_char_array(char* buffer, size_t len){
  for(size_t i=0; i < len; i++){
    buffer[i] = _input_buffer[_input_index + i];
  }
  return moveInputBufferIndex(len*sizeof(char)); //FIXME should be len-1?
}

//stdint types
PacketShared::STATUS PacketCommand::unpack_int8(int8_t& varByRef){
  varByRef = *((int8_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int8_t));
}

PacketShared::STATUS PacketCommand::unpack_uint8(uint8_t& varByRef){
  varByRef = *((uint8_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint8_t));
}

PacketShared::STATUS PacketCommand::unpack_int16(int16_t& varByRef){
  varByRef = *((int16_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int16_t));
}

PacketShared::STATUS PacketCommand::unpack_uint16(uint16_t& varByRef){
  varByRef = *((uint16_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint16_t));
}

PacketShared::STATUS PacketCommand::unpack_int32(int32_t& varByRef){
  varByRef = *((int32_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int32_t));
}

PacketShared::STATUS PacketCommand::unpack_uint32(uint32_t& varByRef){
  varByRef = *((uint32_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint32_t));
}

PacketShared::STATUS PacketCommand::unpack_int64(int64_t& varByRef){
  varByRef = *((int64_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int64_t));
}

PacketShared::STATUS PacketCommand::unpack_uint64(uint64_t& varByRef){
  varByRef = *((uint64_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint64_t));
}

//floating point

PacketShared::STATUS PacketCommand::unpack_float(float& varByRef){
  varByRef = *((float*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(float));
}

PacketShared::STATUS PacketCommand::unpack_double(double& varByRef){
  varByRef = *((double*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(double));
}

PacketShared::STATUS PacketCommand::unpack_float32(float32_t& varByRef){
  varByRef = *((float32_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(float32_t));
}

PacketShared::STATUS PacketCommand::unpack_float64(float64_t& varByRef){
  varByRef = *((float64_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(float64_t));
}

/******************************************************************************/
// Byte field packing methods into output buffer
/******************************************************************************/

//bytes and chars
PacketShared::STATUS PacketCommand::pack_byte(byte value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(byte));
  return moveOutputBufferIndex(sizeof(byte));
}

PacketShared::STATUS PacketCommand::pack_byte_array(byte* buffer, size_t len){
  memcpy( (_output_buffer + _output_index), buffer, len*sizeof(byte)); //FIXME should be len-1?
  return moveOutputBufferIndex(len*sizeof(byte));
}

PacketShared::STATUS PacketCommand::pack_char(char value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(char));
  return moveOutputBufferIndex(sizeof(char));
}

PacketShared::STATUS PacketCommand::pack_char_array(char* buffer, size_t len){
  memcpy( (_output_buffer + _output_index), buffer, len*sizeof(char));
  return moveOutputBufferIndex(len*sizeof(char)); //FIXME should be len-1?
}

//stdint types
PacketShared::STATUS PacketCommand::pack_int8(int8_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int8_t));
  return moveOutputBufferIndex(sizeof(int8_t));
}

PacketShared::STATUS PacketCommand::pack_uint8(uint8_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint8_t));
  return moveOutputBufferIndex(sizeof(uint8_t));
}

PacketShared::STATUS PacketCommand::pack_int16(int16_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int16_t));
  return moveOutputBufferIndex(sizeof(int16_t));
}

PacketShared::STATUS PacketCommand::pack_uint16(uint16_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint16_t));
  return moveOutputBufferIndex(sizeof(uint16_t));
}

PacketShared::STATUS PacketCommand::pack_int32(int32_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int32_t));
  return moveOutputBufferIndex(sizeof(int32_t));
}

PacketShared::STATUS PacketCommand::pack_uint32(uint32_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint32_t));
  return moveOutputBufferIndex(sizeof(uint32_t));
}

PacketShared::STATUS PacketCommand::pack_int64(int64_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int64_t));
  return moveOutputBufferIndex(sizeof(int64_t));
}

PacketShared::STATUS PacketCommand::pack_uint64(uint64_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint64_t));
  return moveOutputBufferIndex(sizeof(uint64_t));
}

//floating point

PacketShared::STATUS PacketCommand::pack_float(float value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(float));
  return moveOutputBufferIndex(sizeof(float));
}

PacketShared::STATUS PacketCommand::pack_double(double value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(double));
  return moveOutputBufferIndex(sizeof(double));
}

PacketShared::STATUS PacketCommand::pack_float32(float32_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(float32_t));
  return moveOutputBufferIndex(sizeof(float32_t));
}

PacketShared::STATUS PacketCommand::pack_float64(float64_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(float64_t));
  return moveOutputBufferIndex(sizeof(float64_t));
}

