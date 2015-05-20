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
    static const int MAXCOMMANDS_DEFAULT = 10;
    static const int MAX_TYPE_ID_LEN = 4;
    static const int INPUTBUFFERSIZE_DEFAULT = 64;
    static const int OUTPUTBUFFERSIZE_DEFAULT = 64;
    // Status and Error  Codes
    typedef enum StatusCode {
      SUCCESS = 0,
      ERROR_EXCEDED_MAX_COMMANDS  = -1,
      ERROR_NO_COMMAND_NAME_MATCH = -2,
      ERROR_INVALID_PACKET        = -3,
      ERROR_INVALID_TYPE_ID       = -4,
      ERROR_NO_TYPE_ID_MATCH      = -5,
      ERROR_NULL_HANDLER_FUNCTION_POINTER = -6,
      ERROR_PACKET_INDEX_OUT_OF_BOUNDS = -7
    } STATUS;
    // Command/handler info structure
    struct CommandInfo {
      byte type_id[MAX_TYPE_ID_LEN];     //limited size type ID must be respected!
      const char* name;
      void (*function)(PacketCommand);     //handler callback function
    };
    // Constructor
    PacketCommand(size_t maxCommands      = MAXCOMMANDS_DEFAULT,
                  size_t inputBufferSize = INPUTBUFFERSIZE_DEFAULT,
                  size_t outputBufferSize = OUTPUTBUFFERSIZE_DEFAULT
                 );
    STATUS addCommand(const byte* type_id,
                      const char* name, 
                      void(*function)(PacketCommand));                          // Add a command to the processing dictionary.
    STATUS registerDefaultHandler(void (*function)(PacketCommand));             // A handler to call when no valid command received.
    //callbacks for IO steps
    STATUS registerBeginDialogCallback(void (*function)(void));                 // A callback which starts a dialog
    STATUS registerWriteCallback(void (*function)(byte* pkt, size_t len));      // A callback which writes output to the interface
    STATUS registerGetReplyCallback(void (*function)(byte* buf, size_t len));   // A callback which waits for a reply
    STATUS registerEndDialogCallback(void (*function)(void));                   // A callback which ends a dialog
    //--
    STATUS lookupCommandByName(const char* name);                        //lookup and set current command by name
    CommandInfo getCurrentCommand();
    STATUS recv(byte* pkt, size_t len);                // Read packet header and locate a matching registered handler function
    STATUS dispatch();
    STATUS beginDialog();
    STATUS send();                                     // Use the '_write_callback' to send OutputBuffer
    STATUS exchange();                                 // Use the '_write_callback' to send OutputBuffer and wait for reply which gets put in InputBuffer
    STATUS endDialog();
    
    
    int    getInputBufferIndex();
    size_t getInputLen(){return _input_len;};
    byte*  getInputBuffer(){return _input_buffer;};
    byte*  getOutputBuffer(){return _output_data_ptr;};
    STATUS setInputBufferIndex(int new_index);
    STATUS moveInputBufferIndex(int n);
    int    getOutputBufferIndex();
    STATUS setOutputBufferIndex(int new_index);
    STATUS moveOutputBufferIndex(int n);
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
    CommandInfo *_commandList;    //array to hold command entries
    CommandInfo _current_command; //command ready to dispatch
    CommandInfo _default_command; //called when a packet's Type ID is not recognized
    int     _commandCount;
    int     _maxCommands;
    //track state of input buffer
    size_t _inputBufferSize;
    byte*  _input_buffer;        //this will be a fixed buffer location
    byte*  _input_data_ptr;      //this can point to _input_buffer or another user buffer
    int    _input_index;
    size_t _input_len;
    //track state of output buffer
    size_t _outputBufferSize;
    byte*  _output_buffer;       //this will be a fixed buffer location
    byte*  _output_data_ptr;     //this can point to _output_buffer or another user buffer
    int    _output_index;
    size_t _output_len;
    //cached callbacks
    void (*_begin_dialog_callback)(void);
    void (*_write_callback)(byte* pkt, size_t len);
    void (*_get_reply_callback)(byte* buf, size_t len);
    void (*_end_dialog_callback)(void);
    

};

#endif //PACKETCOMMAND_H
