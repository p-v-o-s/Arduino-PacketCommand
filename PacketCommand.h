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

#include "PacketQueue.h"
#include "PacketShared.h"

// Uncomment the next line to run the library in debug mode (verbose messages)
//#define PACKETCOMMAND_DEBUG

#ifndef DEBUG_PORT
  #define DEBUG_PORT Serial3
#endif

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
    static const size_t INPUTBUFFERSIZE_DEFAULT = 64;   //FIXME zero means do not allocate
    static const size_t OUTPUTBUFFERSIZE_DEFAULT = 64;
    
    // Command/handler info structure
    struct CommandInfo {
      byte type_id[MAX_TYPE_ID_LEN];     //limited size type ID must be respected!
      const char* name;
      void (*function)(PacketCommand&);     //handler callback function
    };
    
    // Constructor
    PacketCommand(size_t maxCommands      = MAXCOMMANDS_DEFAULT,
                  size_t inputBufferSize  = INPUTBUFFERSIZE_DEFAULT,
                  size_t outputBufferSize = OUTPUTBUFFERSIZE_DEFAULT
                 );
    PacketShared::STATUS reset();
    PacketShared::STATUS addCommand(const byte* type_id,
                      const char* name, 
                      void(*function)(PacketCommand&));                          // Add a command to the processing dictionary.
    PacketShared::STATUS registerDefaultHandler(void (*function)(PacketCommand&));             // A handler to call when no valid command received.
    //registering callbacks for IO steps
    //input
    PacketShared::STATUS registerRecvCallback(bool (*function)(PacketCommand&));
    PacketShared::STATUS registerReplySendCallback(void (*function)(PacketCommand&));
    //output
    PacketShared::STATUS registerSendCallback(bool (*function)(PacketCommand&));                  // A callback which writes output to the interface
    PacketShared::STATUS registerSendNonblockingCallback(void (*function)(PacketCommand&));       // A callback which schedules to writes output to the interface, returns immediately
    PacketShared::STATUS registerSendBufferedCallback(void (*function)(PacketCommand&));   // A callback which schedules to writes output to the interface's buffer, returns immediately
    PacketShared::STATUS registerReplyRecvCallback(bool (*function)(PacketCommand&));
    
    PacketShared::STATUS processInput();  //receive input, match command, and dispatch
    
    PacketShared::STATUS lookupCommandByName(const char* name);                               //lookup and set current command by name
    CommandInfo getCurrentCommand();
    PacketShared::STATUS recv();                // Use the '_recv_callback' to put data into _input_buffer
    PacketShared::STATUS recv(bool& gotPacket); // Use the '_recv_callback' to put data into _input_buffer
    PacketShared::STATUS set_recvTimestamp(uint32_t timestamp_micros);
    uint32_t             get_recvTimestamp(){return _recv_timestamp_micros;};
    PacketShared::STATUS matchCommand();        // Read the packet header from the input buffer and locate a matching registered handler function
    PacketShared::STATUS dispatchCommand();     // Call the current Command
    PacketShared::STATUS send();                // Use the '_send_callback' to send _output_buffer
    PacketShared::STATUS send(bool& sentPacket);// Use the '_send_callback' to send _output_buffer
    PacketShared::STATUS send_nonblocking();    // Use the '_send_nonblocking_callback' to send _schedule the output_buffer contents to be sent, returning immediately
    PacketShared::STATUS send_buffered();       // Use the '_send_buffered_callback' to send _schedule the output_buffer contents to be sent, returning immediately
    PacketShared::STATUS set_sendTimestamp(uint32_t timestamp_micros);
    PacketShared::STATUS reply_send();
    PacketShared::STATUS reply_recv();
    
    PacketShared::STATUS assignInputBuffer(byte* buff, size_t len);
    byte*  getInputBuffer();
    int    getInputBufferIndex();
    size_t getInputLen(){return _input_len;};
    PacketShared::STATUS setInputBufferIndex(int new_index);
    PacketShared::STATUS moveInputBufferIndex(int n);
    void   resetInputBuffer();
    PacketShared::STATUS enqueueInputBuffer(PacketQueue& pq);
    PacketShared::STATUS dequeueInputBuffer(PacketQueue& pq);
    byte*  getOutputBuffer(){return _output_buffer;};
    int    getOutputBufferIndex();
    size_t getOutputLen(){return _output_len;};
    PacketShared::STATUS setOutputBufferIndex(int new_index);
    byte   getOutputFlags(){return _output_flags;};
    void   flagOutputAsQuery(){_output_flags|=PacketShared::OPFLAG_IS_QUERY;};
    void   flagOutputAppendSendTimestamp(){_output_flags|=PacketShared::OPFLAG_APPEND_SEND_TIMESTAMP;};
    bool   outputIsQuery(){return (bool)_output_flags&PacketShared::OPFLAG_IS_QUERY;};
    PacketShared::STATUS enqueueOutputBuffer(PacketQueue& pq);
    PacketShared::STATUS dequeueOutputBuffer(PacketQueue& pq);
    PacketShared::STATUS requeueOutputBuffer(PacketQueue& pq);
    PacketShared::STATUS moveOutputBufferIndex(int n);
    void   resetOutputBuffer();
    //unpacking chars and bytes
    PacketShared::STATUS unpack_byte(byte& varByRef);
    PacketShared::STATUS unpack_byte_array(byte* buffer, size_t len);
    PacketShared::STATUS unpack_char(char& varByRef);
    PacketShared::STATUS unpack_char_array(char* buffer, size_t len);
    //unpacking stdint types
    PacketShared::STATUS unpack_int8(    int8_t& varByRef);
    PacketShared::STATUS unpack_uint8(  uint8_t& varByRef);
    PacketShared::STATUS unpack_int16(  int16_t& varByRef);
    PacketShared::STATUS unpack_uint16(uint16_t& varByRef);
    PacketShared::STATUS unpack_int32(  int32_t& varByRef);
    PacketShared::STATUS unpack_uint32(uint32_t& varByRef);
    PacketShared::STATUS unpack_int64(  int64_t& varByRef);
    PacketShared::STATUS unpack_uint64(uint64_t& varByRef);
    //unpacking floating point types
    PacketShared::STATUS unpack_float(        float& varByRef);
    PacketShared::STATUS unpack_double(      double& varByRef);
    PacketShared::STATUS unpack_float32(  float32_t& varByRef);
    PacketShared::STATUS unpack_float64(  float64_t& varByRef);
    //Methods for constructing an output
    PacketShared::STATUS setupOutputCommandByName(const char* name);
    PacketShared::STATUS setupOutputCommand(CommandInfo command);
    //packing chars and bytes
    PacketShared::STATUS pack_byte(byte value);
    PacketShared::STATUS pack_byte_array(byte* buffer, size_t len);
    PacketShared::STATUS pack_char(char value);
    PacketShared::STATUS pack_char_array(char* buffer, size_t len);
    //packing stdint types
    PacketShared::STATUS pack_int8(    int8_t value);
    PacketShared::STATUS pack_uint8(  uint8_t value);
    PacketShared::STATUS pack_int16(  int16_t value);
    PacketShared::STATUS pack_uint16(uint16_t value);
    PacketShared::STATUS pack_int32(  int32_t value);
    PacketShared::STATUS pack_uint32(uint32_t value);
    PacketShared::STATUS pack_int64(  int64_t value);
    PacketShared::STATUS pack_uint64(uint64_t value);
    //packing floating point types
    PacketShared::STATUS pack_float(        float value);
    PacketShared::STATUS pack_double(      double value);
    PacketShared::STATUS pack_float32(  float32_t value);
    PacketShared::STATUS pack_float64(  float64_t value);

  private:
    //helper methods
    void allocateInputBuffer(size_t len);
    void allocateOutputBuffer(size_t len);
    //data members
    CommandInfo *_commandList;    //array to hold command entries
    CommandInfo _current_command; //command ready to dispatch
    CommandInfo _default_command; //called when a packet's Type ID is not recognized
    size_t  _commandCount;
    size_t  _maxCommands;
    //track state of input buffer
    size_t   _inputBufferSize;
    byte*    _input_buffer;        //this will be a fixed buffer location
    size_t   _input_index;
    size_t   _input_len;
    byte     _input_flags;
    uint32_t _recv_timestamp_micros;
    //track state of output buffer
    size_t _outputBufferSize;
    byte*  _output_buffer;       //this will be a fixed buffer location
    size_t _output_index;
    size_t _output_len;
    byte   _output_flags;
    uint32_t _send_timestamp_micros;
    //cached callbacks
    bool (*_send_callback)(PacketCommand& this_pCmd);
    void (*_send_nonblocking_callback)(PacketCommand& this_pCmd);
    void (*_send_buffered_callback)(PacketCommand& this_pCmd);
    bool (*_recv_callback)(PacketCommand& this_pCmd);
    void (*_reply_send_callback)(PacketCommand& this_pCmd);
    bool (*_reply_recv_callback)(PacketCommand& this_pCmd);

};

#endif //PACKETCOMMAND_H
