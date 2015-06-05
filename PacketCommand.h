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
#include <stdint.h>

// Uncomment the next line to run the library in debug mode (verbose messages)
//#define PACKETCOMMAND_DEBUG

typedef float  float32_t;
typedef double float64_t;

/******************************************************************************/
// PacketCommand
// so that callbacks print
class PacketCommand{
  public:
    // Constants
    static const size_t MAXCOMMANDS_DEFAULT = 10;
    static const size_t MAX_TYPE_ID_LEN = 4;
    static const size_t INPUTBUFFERSIZE_DEFAULT = 0;   //zero means do not allocate
    static const size_t OUTPUTBUFFERSIZE_DEFAULT = 64;
    static const size_t INPUTQUEUESIZE_DEFAULT = 3;
    static const size_t OUTPUTQUEUESIZE_DEFAULT = 3;
    // Status and Error  Codes
    typedef enum StatusCode {
      NO_PACKET_RECEIVED          = 1,
      SUCCESS = 0,
      ERROR_EXCEDED_MAX_COMMANDS  = -1,
      ERROR_NO_COMMAND_NAME_MATCH = -2,
      ERROR_INVALID_PACKET        = -3,
      ERROR_INVALID_TYPE_ID       = -4,
      ERROR_NO_TYPE_ID_MATCH      = -5,
      ERROR_NULL_HANDLER_FUNCTION_POINTER = -6,
      ERROR_PACKET_INDEX_OUT_OF_BOUNDS = -7,
      ERROR_INPUT_BUFFER_OVERRUN  = -8,
      ERROR_QUEUE_OVERFLOW        = -9,
      ERROR_QUEUE_UNDERFLOW       = -10
    } STATUS;
    // Command/handler info structure
    struct CommandInfo {
      byte type_id[MAX_TYPE_ID_LEN];     //limited size type ID must be respected!
      const char* name;
      void (*function)(PacketCommand&);     //handler callback function
    };
    // Packet structure
    struct Packet {
      size_t length;
      byte*    data;
      bool is_query;
    };
    // Constructor
    PacketCommand(size_t maxCommands      = MAXCOMMANDS_DEFAULT,
                  size_t inputBufferSize  = INPUTBUFFERSIZE_DEFAULT,
                  size_t outputBufferSize = OUTPUTBUFFERSIZE_DEFAULT,
                  size_t inputQueueSize   = INPUTQUEUESIZE_DEFAULT,
                  size_t outputQueueSize  = OUTPUTQUEUESIZE_DEFAULT
                 );
    STATUS addCommand(const byte* type_id,
                      const char* name, 
                      void(*function)(PacketCommand&));                          // Add a command to the processing dictionary.
    STATUS registerDefaultHandler(void (*function)(PacketCommand&));             // A handler to call when no valid command received.
    //registering callbacks for IO steps
    //input
    STATUS registerRecvCallback(bool (*function)(PacketCommand&));
    STATUS registerReplySendCallback(void (*function)(PacketCommand&));
    //output
    STATUS registerSendCallback(void (*function)(PacketCommand&));              // A callback which writes output to the interface
    STATUS registerSendNonblockingCallback(void (*function)(PacketCommand&));   // A callback which schedules to writes output to the interface, returns immediately
    STATUS registerReplyRecvCallback(bool (*function)(PacketCommand&));
    
    STATUS processInput();  //receive input, match command, and dispatch
    
    STATUS lookupCommandByName(const char* name);                        //lookup and set current command by name
    CommandInfo getCurrentCommand();

    STATUS recv();                // Use the '_recv_callback' to put data into _input_buffer
    STATUS matchCommand();        // Read the packet header from the input buffer and locate a matching registered handler function
    STATUS dispatchCommand();     // Call the current Command
    STATUS send();                // Use the '_send_callback' to send _output_buffer
    STATUS send_nonblocking();    // Use the '_send_nonblocking_callback' to send _schedule the output_buffer contents to be sent, returning immediately
    STATUS reply_send();
    STATUS reply_recv();
    
    STATUS assignInputBuffer(byte* buff, size_t len);
    byte*  getInputBuffer();
    int    getInputBufferIndex();
    size_t getInputLen(){return _input_len;};
    STATUS setInputBufferIndex(int new_index);
    STATUS moveInputBufferIndex(int n);
    void   resetInputBuffer(){_input_index=0;_input_len=0;};
    STATUS enqueueInputBuffer();
    STATUS dequeueInputBuffer();
    int    getInputQueueIndex(){return _input_queue_index;}
    byte*  getOutputBuffer(){return _output_buffer;};
    int    getOutputBufferIndex();
    size_t getOutputLen(){return _output_len;};
    STATUS setOutputBufferIndex(int new_index);
    STATUS markOutputAsQuery(){_output_is_query=true;return SUCCESS;}
    bool   isOutputQuery(){return _output_is_query;}
    STATUS enqueueOutputBuffer();
    STATUS dequeueOutputBuffer();
    STATUS requeueOutputBuffer();
    int    getOutputQueueIndex(){return _output_queue_index;}
    STATUS moveOutputBufferIndex(int n);
    void   resetOutputBuffer(){_output_index=0;_output_len=0;};
    //unpacking chars and bytes
    STATUS unpack_byte(byte& varByRef);
    STATUS unpack_byte_array(byte* buffer, int len);
    STATUS unpack_char(char& varByRef);
    STATUS unpack_char_array(char* buffer, int len);
    //unpacking stdint types
    STATUS unpack_int8(    int8_t& varByRef);
    STATUS unpack_uint8(  uint8_t& varByRef);
    STATUS unpack_int16(  int16_t& varByRef);
    STATUS unpack_uint16(uint16_t& varByRef);
    STATUS unpack_int32(  int32_t& varByRef);
    STATUS unpack_uint32(uint32_t& varByRef);
    STATUS unpack_int64(  int64_t& varByRef);
    STATUS unpack_uint64(uint64_t& varByRef);
    //unpacking floating point types
    STATUS unpack_float(        float& varByRef);
    STATUS unpack_double(      double& varByRef);
    STATUS unpack_float32(  float32_t& varByRef);
    STATUS unpack_float64(  float64_t& varByRef);
    //Methods for constructing an output
    STATUS setupOutputCommandByName(const char* name);
    //packing chars and bytes
    STATUS pack_byte(byte value);
    STATUS pack_byte_array(byte* buffer, int len);
    STATUS pack_char(char value);
    STATUS pack_char_array(char* buffer, int len);
    //packing stdint types
    STATUS pack_int8(    int8_t value);
    STATUS pack_uint8(  uint8_t value);
    STATUS pack_int16(  int16_t value);
    STATUS pack_uint16(uint16_t value);
    STATUS pack_int32(  int32_t value);
    STATUS pack_uint32(uint32_t value);
    STATUS pack_int64(  int64_t value);
    STATUS pack_uint64(uint64_t value);
    //packing floating point types
    STATUS pack_float(        float value);
    STATUS pack_double(      double value);
    STATUS pack_float32(  float32_t value);
    STATUS pack_float64(  float64_t value);

  private:
    //helper methods
    void allocateInputBuffer(size_t len);
    //data members
    CommandInfo *_commandList;    //array to hold command entries
    CommandInfo _current_command; //command ready to dispatch
    CommandInfo _default_command; //called when a packet's Type ID is not recognized
    int     _commandCount;
    int     _maxCommands;
    //track state of input buffer
    size_t _inputBufferSize;
    byte*  _input_buffer;        //this will be a fixed buffer location
    int    _input_index;
    size_t _input_len;
    size_t _inputQueueSize;     //limit to input Queue
    Packet** _input_queue;
    int    _input_queue_index;
    //track state of output buffer
    size_t _outputBufferSize;
    byte*  _output_buffer;       //this will be a fixed buffer location
    int    _output_index;
    size_t _output_len;
    bool   _output_is_query = false;
    size_t _outputQueueSize;     //limit to input Queue
    Packet** _output_queue;
    int    _output_queue_index;
    //cached callbacks
    void (*_send_callback)(PacketCommand& this_pCmd);
    void (*_send_nonblocking_callback)(PacketCommand& this_pCmd);
    bool (*_recv_callback)(PacketCommand& this_pCmd);
    void (*_reply_send_callback)(PacketCommand& this_pCmd);
    bool (*_reply_recv_callback)(PacketCommand& this_pCmd);

};

#endif //PACKETCOMMAND_H
