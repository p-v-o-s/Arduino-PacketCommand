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
                             size_t inputBufferSize,
                             size_t outputBufferSize,
                             size_t inputQueueSize,
                             size_t outputQueueSize
                            )
  : _commandCount(0)
{
  _maxCommands = maxCommands;
  //allocate memory for the command lookup and intialize with all null pointers
  _commandList = (CommandInfo*) calloc(maxCommands, sizeof(CommandInfo));
  //allocate memory for the input buffer
  _inputBufferSize = inputBufferSize;
  if (_inputBufferSize > 0){ //allocate memory
     allocateInputBuffer(inputBufferSize);
  }
  else{ //do not allocate anything
    _input_buffer = NULL;
  }
  _input_index = 0;
  _input_len   = 0;
  _inputQueueSize = inputQueueSize;     //limit to input Queue
  //preallocate memory for input queue
  _input_queue = (Packet**) calloc(_inputQueueSize, sizeof(Packet**));
  for(size_t i=0; i < _inputQueueSize; i++){
    struct Packet *pkt = (struct Packet *) calloc(1, sizeof(struct Packet*));
    pkt->length = _input_len;
    pkt->data   = (byte*) calloc(_inputBufferSize, sizeof(byte*));
    _input_queue[i] = pkt;
  }
  _input_queue_index = -1; //start out empty
  //allocate memory for the output buffer
  _outputBufferSize = outputBufferSize;
  _output_buffer   = (byte*) calloc(outputBufferSize, sizeof(byte));
  _output_index = 0;
  _output_len   = 0;
  _outputQueueSize = outputQueueSize;     //limit to input Queue
  //preallocate memory for input queue
  _output_queue = (Packet**) calloc(_outputQueueSize, sizeof(Packet**));
  for(size_t i=0; i < _outputQueueSize; i++){
    struct Packet *pkt = (struct Packet *) calloc(1, sizeof(struct Packet*));
    pkt->length = _output_len;
    pkt->data   = (byte*) calloc(_outputBufferSize, sizeof(byte*));
    _output_queue[i] = pkt;
  }
  _output_queue_index = -1; //start out empty
  //null out unregistered callbacks
  _begin_input_callback = NULL;
  _recv_callback = NULL;
  _reply_send_callback = NULL;
  _end_input_callback = NULL;
  _begin_output_callback = NULL;
  _send_callback = NULL;
  _reply_recv_callback = NULL;
  _end_output_callback = NULL;
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
                                                void (*function)(PacketCommand&)) {
  byte cur_byte = 0x00;
  size_t type_id_len = strlen((char*) type_id);
  struct CommandInfo new_command;
  #ifdef PACKETCOMMAND_DEBUG
  Serial.print(F("Adding command #("));
  Serial.print(_commandCount);
  Serial.print(F("): "));
  Serial.println(name);
  Serial.print("'");
  Serial.print((char *) type_id);
  Serial.println("'");
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
  for(size_t i=0; i < MAX_TYPE_ID_LEN; i++){
    if (i < type_id_len){
      //test if the type ID rules are followed
      cur_byte = type_id[i];
      #ifdef PACKETCOMMAND_DEBUG
      Serial.print(F("i="));
      Serial.println(i);
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
          break;
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
PacketCommand::STATUS PacketCommand::registerDefaultHandler(void (*function)(PacketCommand&)) {
  if (function != NULL){
    _default_command.function = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * 
 */
PacketCommand::STATUS PacketCommand::registerBeginInputCallback(void (*function)(PacketCommand&)){
  if (function != NULL){
    _begin_input_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to read a 
 * packet into its input buffer
 */
PacketCommand::STATUS PacketCommand::registerRecvCallback(bool (*function)(PacketCommand&)){
  if (function != NULL){
    _recv_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to send a 
 * packet from its output buffer
 */
PacketCommand::STATUS PacketCommand::registerReplySendCallback(void (*function)(PacketCommand&)){
  if (function != NULL){
    _reply_send_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * 
 */
PacketCommand::STATUS PacketCommand::registerEndInputCallback(void (*function)(PacketCommand&)){
  if (function != NULL){
    _end_input_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * 
 */
PacketCommand::STATUS PacketCommand::registerBeginOutputCallback(void (*function)(PacketCommand&)){
  if (function != NULL){
    _begin_output_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * This sets up a callback which can be used by a command handler to send a 
 * packet from its output buffer
 */
PacketCommand::STATUS PacketCommand::registerSendCallback(void (*function)(PacketCommand&)){
  if (function != NULL){
    _send_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * 
 */
PacketCommand::STATUS PacketCommand::registerReplyRecvCallback(bool (*function)(PacketCommand&)){
  if (function != NULL){
    _reply_recv_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

/**
 * 
 */
PacketCommand::STATUS PacketCommand::registerEndOutputCallback(void (*function)(PacketCommand&)){
  if (function != NULL){
    _end_output_callback = function;
    return SUCCESS;
  }
  else{
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

PacketCommand::STATUS PacketCommand::processInput(){
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::processInput"));
  Serial.print(F("\t_input_index="));Serial.println(_input_index);
  Serial.print(F("\t_input_len="));Serial.println(_input_len);
  #endif
  STATUS pcs = matchCommand();
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("(processInput)-after calling matchCommand()"));
  Serial.print(F("\t_input_index="));Serial.println(_input_index);
  Serial.print(F("\t_input_len="));Serial.println(_input_len);
  #endif
  if (pcs == SUCCESS){  //a command was matched
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("(processInput)-matched command: "));
    CommandInfo cmd = getCurrentCommand();
    Serial.println(cmd.name);
    #endif
  }
  else if (pcs == ERROR_NO_TYPE_ID_MATCH){  //valid ID but no command was matched
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("(processInput)-no matched command"));
    #endif
  }
  else{
    Serial.print(F("### Error: pCmd.matchCommand returned status code: "));
    Serial.println(pcs);
    return pcs;
  }
  //dispatch to handler or default if no match
  dispatchCommand();
  return SUCCESS;
}


PacketCommand::STATUS PacketCommand::lookupCommandByName(const char* name){
  _current_command = _default_command;
  #ifdef PACKETCOMMAND_DEBUG
  Serial.print(F("Searching for command named = "));
  Serial.println(name);
  #endif
  for(size_t i=0; i <= _maxCommands; i++){
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

// Begin an input phase by prepareing input buffer and calling back to client
PacketCommand::STATUS PacketCommand::beginInput(){
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::beginInput()"));
  Serial.print(F("\t_input_index="));Serial.println(_input_index);
  Serial.print(F("\t_input_len="));Serial.println(_input_len);
  #endif
  _input_index = 0;
  _input_len = 0;
  //call if it exists
  if (_begin_input_callback != NULL){
    (*_begin_input_callback)(*this);
  }
  return SUCCESS;
}

PacketCommand::STATUS PacketCommand::recv() {
  bool gotPacket;
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::recv()"));
  Serial.print(F("\t_input_index="));Serial.println(_input_index);
  Serial.print(F("\t_input_len="));Serial.println(_input_len);
  #endif
  //call the read callback which should load data into _input_buffer and set len
  if (_recv_callback != NULL){
    gotPacket = (*_recv_callback)(*this);
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("Error: tried write using a NULL read callback function pointer"));
    #endif
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
  if (gotPacket){ //we have a packet
    return SUCCESS;
  }
  else{  //we have no packet
    return NO_PACKET_RECEIVED;
  }
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
 * such that it is not NULL; otherwise, we return ERROR_NO_TYPE_ID_MATCH.  If return is SUCCESS, then
 * the packet buffer position will have been moved past the type ID field to prepare for parsing any 
 * following binary fields; otherwise, the packet buffer position will remain at the byte that caused 
 * the error condition.
 */
PacketCommand::STATUS PacketCommand::matchCommand(){
  byte cur_byte = 0x00;
  _current_command = _default_command;
  //parse out type_id from header
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::matchCommand"));
  Serial.print(F("\t_input_index="));Serial.println(_input_index);
  Serial.print(F("\t_input_len="));Serial.println(_input_len);
  #endif
  while(_input_index < (int) _input_len){
    cur_byte = _input_buffer[_input_index];
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
PacketCommand::STATUS PacketCommand::dispatchCommand() {
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::dispatchCommand"));
  #endif
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

//use unpack* methods to pull out data from packet

// End an input phase by calling back to client
PacketCommand::STATUS PacketCommand::endInput(){
  //call if it exists
  if (_end_input_callback != NULL){
    (*_end_input_callback)(*this);
  }
  return SUCCESS;
}

// Begin an input phase by prepareing input buffer and calling back to client
PacketCommand::STATUS PacketCommand::beginOutput(){
  _output_index = 0;
  _output_len = 0;
  //call if it exists
  if (_begin_output_callback != NULL){
    (*_begin_output_callback)(*this);
  }
  return SUCCESS;
}

PacketCommand::STATUS PacketCommand::setupOutputCommandByName(const char* name){
  STATUS pcs;
  byte cur_byte;
  pcs = lookupCommandByName(name);  //sets _current_command on SUCCESS
  //reset output buffer state
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

//use pack* methods to add additional arguments to output buffer

// Use the '_send_callback' to send return packet
PacketCommand::STATUS PacketCommand::send(){
  if (_send_callback != NULL){
    //call the send callback
    (*_send_callback)(*this);
    return SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("Error: tried to send using a NULL send callback function pointer"));
    #endif
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}

// Begin the dialog by prepareing input and output buffers and calling back to client
PacketCommand::STATUS PacketCommand::endOutput(){
  //call if it exists
  if (_end_output_callback != NULL){
    (*_end_output_callback)(*this);
  }
  return SUCCESS;
}


// Use the '_reply_send_callback' to send a quick reply
PacketCommand::STATUS PacketCommand::reply_send(){
  if (_reply_send_callback != NULL){
    //call the send callback
    (*_reply_send_callback)(*this);
    return SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("Error: tried to send using a NULL send callback function pointer"));
    #endif
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}


// Use the '_reply_send_callback' to send a quick reply
PacketCommand::STATUS PacketCommand::reply_recv(){
  if (_reply_recv_callback != NULL){
    //call the send callback
    (*_reply_recv_callback)(*this);
    return SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("Error: tried to receive using a NULL recv callback function pointer"));
    #endif
    return ERROR_NULL_HANDLER_FUNCTION_POINTER;
  }
}


/**
 * Accessors and mutators for the input buffer
*/

PacketCommand::STATUS PacketCommand::assignInputBuffer(byte* buff, size_t len){
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::assignInputBuffer"));
  Serial.print(F("\tlen="));Serial.println(len);
  Serial.print(F("\t_inputBufferSize="));Serial.println(_inputBufferSize);
  Serial.print(F("\t_input_index="));Serial.println(_input_index);
  Serial.print(F("\t_input_len="));Serial.println(_input_len);
  #endif
  _input_buffer = buff;
  //check the input length before setting
  if (len <= _inputBufferSize){
    _input_len = len;
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("(assignInputBuffer) after setting _input_len"));
    Serial.print(F("\t_input_index="));Serial.println(_input_index);
    Serial.print(F("\t_input_len="));Serial.println(_input_len);
    #endif
    return SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("Error: tried to receive data that would overrun input buffer"));
    #endif
    _input_len = _inputBufferSize; //set to safe value
    return ERROR_INPUT_BUFFER_OVERRUN;
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

PacketCommand::STATUS PacketCommand::enqueueInputBuffer(){
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::enqueueInputBuffer"));
  #endif
  noInterrupts(); //ensure that queue operations are consistent
  if (_input_queue_index + 1 < _inputQueueSize){
    _input_queue_index++;
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("\tqueueing at index:"));Serial.println(_input_queue_index);
    Serial.print(F(" copying data: "));
    #endif
    struct Packet *pkt = _input_queue[_input_queue_index];
    //copy the current input buffer to the new packet
    for(int i=0; i < _input_len; i++){
      #ifdef PACKETCOMMAND_DEBUG
      Serial.print(_input_buffer[i], HEX);Serial.print(F(" "));
      #endif
      pkt->data[i] = _input_buffer[i];
    }
    pkt->length = _input_len; //update length field
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println();
    #endif
    interrupts(); //restore interrupts
    return SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("\t### Error: Queue Overflow"));
    #endif
    interrupts(); //restore interrupts
    return ERROR_QUEUE_OVERFLOW;
  }
}

PacketCommand::STATUS PacketCommand::dequeueInputBuffer(){
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::dequeueInputBuffer"));
  #endif
  noInterrupts(); //ensure that queue operations are consistent
  if (_input_queue_index >= 0){
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("\tdequeueing from the front"));
    Serial.print(F(" copying data: "));
    #endif
    //grab the first packet
    struct Packet *pkt = _input_queue[0];
    //copy the packet to the current input buffer
    for(int i=0; (i < pkt->length) && (i < _inputBufferSize); i++){
      #ifdef PACKETCOMMAND_DEBUG
      Serial.print(pkt->data[i], HEX);Serial.print(F(" "));
      #endif
      _input_buffer[i] = pkt->data[i];
    }
    //restore buffer state
    _input_index = 0;
    _input_len = min(pkt->length,_inputBufferSize);
    //move queue elements down
    for(int j=1; j < _inputQueueSize; j++){
      _input_queue[j-1] = _input_queue[j];
    }
    //reuse the first pointer at last slot of the queue
    _input_queue[_inputQueueSize-1] = pkt;
    _input_queue_index--;
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println();
    Serial.println(F("(dequeueInputBuffer) after copy"));
    Serial.print(F("\t_input_index="));Serial.println(_input_index);
    Serial.print(F("\t_input_len="));Serial.println(_input_len);
    Serial.print(F("\t_input_queue_index="));Serial.println(_input_queue_index);
    
    #endif
    interrupts(); //restore interrupts
    return SUCCESS;
  }
  else{
     #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("\t### Error: Queue Underflow"));
    #endif
    interrupts(); //restore interrupts
    return ERROR_QUEUE_UNDERFLOW;
  }
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

PacketCommand::STATUS PacketCommand::enqueueOutputBuffer(){
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::enqueueOutputBuffer"));
  Serial.print(F("\t_output_queue_index="));Serial.println(_output_queue_index);
  Serial.print(F("\t_outputQueueSize="));Serial.println(_outputQueueSize);
  #endif
  noInterrupts(); //ensure that queue operations are consistent
  if (_output_queue_index + 1 < _outputQueueSize){
    _output_queue_index++;
    #ifdef PACKETCOMMAND_DEBUG
    Serial.print(F("\tqueueing at index:"));Serial.println(_output_queue_index);
    Serial.print(F(" copying data: "));
    #endif
    struct Packet *pkt = _output_queue[_output_queue_index];
    //copy the current output buffer to the new packet
    for(int i=0; i < _output_len; i++){
      #ifdef PACKETCOMMAND_DEBUG
      Serial.print(_output_buffer[i], HEX);Serial.print(F(" "));
      #endif
      pkt->data[i] = _output_buffer[i];
    }
    pkt->length   = _output_len; //update length field
    pkt->is_query = _output_is_query; 
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println();
    #endif
    interrupts(); //restore interrupts
    return SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("\t### Error: Queue Overflow"));
    #endif
    interrupts(); //restore interrupts
    return ERROR_QUEUE_OVERFLOW;
  }
}

PacketCommand::STATUS PacketCommand::dequeueOutputBuffer(){
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::dequeueOutputBuffer"));
  #endif
  noInterrupts(); //ensure that queue operations are consistent
  if (_output_queue_index >= 0){
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("\tdequeueing from the front"));
    Serial.print(F(" copying data: "));
    #endif
    //grab the first packet
    struct Packet *pkt = _output_queue[0];
    //copy the packet to the current output buffer
    for(int i=0; (i < pkt->length) && (i < _outputBufferSize); i++){
      #ifdef PACKETCOMMAND_DEBUG
      Serial.print(pkt->data[i], HEX);Serial.print(F(" "));
      #endif
      _output_buffer[i] = pkt->data[i];
    }
    //restore buffer state
    _output_index = 0;
    _output_len = min(pkt->length,_outputBufferSize);
    _output_is_query = pkt->is_query;
    //move queue elements down
    for(int j=1; j < _outputQueueSize; j++){
      _output_queue[j-1] = _output_queue[j];
    }

    //reuse the first pointer at last slot of the queue
    _output_queue[_outputQueueSize-1] = pkt;
    _output_queue_index--;
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println();
    Serial.println(F("(dequeueOutputBuffer) after copy"));
    Serial.print(F("\t_output_index="));Serial.println(_output_index);
    Serial.print(F("\t_output_len="));Serial.println(_output_len);
    Serial.print(F("\t_output_queue_index="));Serial.println(_output_queue_index);
    #endif
    interrupts(); //restore interrupts
    return SUCCESS;
  }
  else{
     #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("\t### Error: Queue Underflow"));
    #endif
    interrupts(); //restore interrupts
    return ERROR_QUEUE_UNDERFLOW;
  }
}

PacketCommand::STATUS PacketCommand::requeueOutputBuffer(){
  //pushes output buffer onto the front of the queue
  #ifdef PACKETCOMMAND_DEBUG
  Serial.println(F("In PacketCommand::requeueOutputBuffer"));
  Serial.print(F("\t_output_queue_index="));Serial.println(_output_queue_index);
  Serial.print(F("\t_outputQueueSize="));Serial.println(_outputQueueSize);
  #endif
  noInterrupts(); //ensure that queue operations are consistent
  if (_output_queue_index + 1 < _outputQueueSize){
    _output_queue_index++;
    //move queue elements up
    for(int j=0; j < _outputQueueSize - 1; j++){
      _output_queue[j+1] = _output_queue[j];
    }
    //grab the front slot of the queue
    struct Packet *pkt = _output_queue[0];
    //copy the current output buffer to the packet slot
    for(int i=0; i < _output_len; i++){
      pkt->data[i] = _output_buffer[i];
    }
    pkt->length = _output_len; //update length field
    pkt->is_query = _output_is_query; 
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("(requeueOutputBuffer) after copy"));
    Serial.print(F("\t_output_index="));Serial.println(_output_index);
    Serial.print(F("\t_output_len="));Serial.println(_output_len);
    Serial.print(F("\t_output_queue_index="));Serial.println(_output_queue_index);
    #endif
    interrupts(); //restore interrupts
    return SUCCESS;
  }
  else{
    #ifdef PACKETCOMMAND_DEBUG
    Serial.println(F("\t### Error: Queue Overflow"));
    #endif
    interrupts(); //restore interrupts
    return ERROR_QUEUE_OVERFLOW;
  }
}


/******************************************************************************/
// Byte field unpacking methods from input buffer
/******************************************************************************/
//bytes and chars
PacketCommand::STATUS PacketCommand::unpack_byte(byte& varByRef){
  varByRef = *((byte*)(_input_buffer+_input_index));
  return moveInputBufferIndex(sizeof(byte));
}

PacketCommand::STATUS PacketCommand::unpack_byte_array(byte* buffer, int len){
  for(int i=0; i < len; i++){
    buffer[i] = _input_buffer[_input_index + i];
  }
  return moveInputBufferIndex(len*sizeof(byte)); //FIXME should be len-1?
}

PacketCommand::STATUS PacketCommand::unpack_char(char& varByRef){
  varByRef = *((char*)(_input_buffer+_input_index));
  return moveInputBufferIndex(sizeof(char));
}

PacketCommand::STATUS PacketCommand::unpack_char_array(char* buffer, int len){
  for(int i=0; i < len; i++){
    buffer[i] = _input_buffer[_input_index + i];
  }
  return moveInputBufferIndex(len*sizeof(char)); //FIXME should be len-1?
}

//stdint types
PacketCommand::STATUS PacketCommand::unpack_int8(int8_t& varByRef){
  varByRef = *((int8_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int8_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint8(uint8_t& varByRef){
  varByRef = *((uint8_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint8_t));
}

PacketCommand::STATUS PacketCommand::unpack_int16(int16_t& varByRef){
  varByRef = *((int16_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int16_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint16(uint16_t& varByRef){
  varByRef = *((uint16_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint16_t));
}

PacketCommand::STATUS PacketCommand::unpack_int32(int32_t& varByRef){
  varByRef = *((int32_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int32_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint32(uint32_t& varByRef){
  varByRef = *((uint32_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint32_t));
}

PacketCommand::STATUS PacketCommand::unpack_int64(int64_t& varByRef){
  varByRef = *((int64_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(int64_t));
}

PacketCommand::STATUS PacketCommand::unpack_uint64(uint64_t& varByRef){
  varByRef = *((uint64_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(uint64_t));
}

//floating point

PacketCommand::STATUS PacketCommand::unpack_float(float& varByRef){
  varByRef = *((float*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(float));
}

PacketCommand::STATUS PacketCommand::unpack_double(double& varByRef){
  varByRef = *((double*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(double));
}

PacketCommand::STATUS PacketCommand::unpack_float32(float32_t& varByRef){
  varByRef = *((float32_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(float32_t));
}

PacketCommand::STATUS PacketCommand::unpack_float64(float64_t& varByRef){
  varByRef = *((float64_t*)(_input_buffer + _input_index));
  return moveInputBufferIndex(sizeof(float64_t));
}

/******************************************************************************/
// Byte field packing methods into output buffer
/******************************************************************************/

//bytes and chars
PacketCommand::STATUS PacketCommand::pack_byte(byte value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(byte));
  return moveOutputBufferIndex(sizeof(byte));
}

PacketCommand::STATUS PacketCommand::pack_byte_array(byte* buffer, int len){
  memcpy( (_output_buffer + _output_index), buffer, len*sizeof(byte)); //FIXME should be len-1?
  return moveOutputBufferIndex(len*sizeof(byte));
}

PacketCommand::STATUS PacketCommand::pack_char(char value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(char));
  return moveOutputBufferIndex(sizeof(char));
}

PacketCommand::STATUS PacketCommand::pack_char_array(char* buffer, int len){
  memcpy( (_output_buffer + _output_index), buffer, len*sizeof(char));
  return moveOutputBufferIndex(len*sizeof(char)); //FIXME should be len-1?
}

//stdint types
PacketCommand::STATUS PacketCommand::pack_int8(int8_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int8_t));
  return moveOutputBufferIndex(sizeof(int8_t));
}

PacketCommand::STATUS PacketCommand::pack_uint8(uint8_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint8_t));
  return moveOutputBufferIndex(sizeof(uint8_t));
}

PacketCommand::STATUS PacketCommand::pack_int16(int16_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int16_t));
  return moveOutputBufferIndex(sizeof(int16_t));
}

PacketCommand::STATUS PacketCommand::pack_uint16(uint16_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint16_t));
  return moveOutputBufferIndex(sizeof(uint16_t));
}

PacketCommand::STATUS PacketCommand::pack_int32(int32_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int32_t));
  return moveOutputBufferIndex(sizeof(int32_t));
}

PacketCommand::STATUS PacketCommand::pack_uint32(uint32_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint32_t));
  return moveOutputBufferIndex(sizeof(uint32_t));
}

PacketCommand::STATUS PacketCommand::pack_int64(int64_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(int64_t));
  return moveOutputBufferIndex(sizeof(int64_t));
}

PacketCommand::STATUS PacketCommand::pack_uint64(uint64_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(uint64_t));
  return moveOutputBufferIndex(sizeof(uint64_t));
}

//floating point

PacketCommand::STATUS PacketCommand::pack_float(float value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(float));
  return moveOutputBufferIndex(sizeof(float));
}

PacketCommand::STATUS PacketCommand::pack_double(double value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(double));
  return moveOutputBufferIndex(sizeof(double));
}

PacketCommand::STATUS PacketCommand::pack_float32(float32_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(float32_t));
  return moveOutputBufferIndex(sizeof(float32_t));
}

PacketCommand::STATUS PacketCommand::pack_float64(float64_t value){
  memcpy( (_output_buffer + _output_index), &value, sizeof(float64_t));
  return moveOutputBufferIndex(sizeof(float64_t));
}

