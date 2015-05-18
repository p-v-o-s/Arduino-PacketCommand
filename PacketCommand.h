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
typedef float  float32_t;
typedef double float64_t;


#define PACKETCOMMAND_MAXCOMMANDS_DEFAULT 10
#define MAX_TYPE_ID_LEN 4
#define PACKETCOMMAND_OUTPUTBUFFERSIZE_DEFAULT 64
// Uncomment the next line to run the library in debug mode (verbose messages)
//#define PACKETCOMMAND_DEBUG

/******************************************************************************/
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
} PACKETCOMMAND_STATUS;

/******************************************************************************/


/******************************************************************************/
// PacketCommand (extends Print)
// so that callbacks print
class PacketCommand{
  public:
     // Command/handler info structure
    struct CommandInfo {
      byte type_id[MAX_TYPE_ID_LEN];     //limited size type ID must be respected!
      const char* name;
      void (*function)(PacketCommand);     //handler callback function
    };
    // Constructor
    PacketCommand(size_t maxCommands      = PACKETCOMMAND_MAXCOMMANDS_DEFAULT,
                  size_t outputBufferSize = PACKETCOMMAND_OUTPUTBUFFERSIZE_DEFAULT
                 );
    PACKETCOMMAND_STATUS addCommand(const byte* type_id,
                                    const char* name, 
                                    void(*function)(PacketCommand));                     // Add a command to the processing dictionary.
    PACKETCOMMAND_STATUS registerDefaultHandler(void (*function)(PacketCommand));        // A handler to call when no valid command received.
    PACKETCOMMAND_STATUS registerWriteCallback(void (*function)(byte* pkt, size_t len)); // A callback which processes the output
    PACKETCOMMAND_STATUS lookupCommandByName(const char* name);                        //lookup and set current command by name
    CommandInfo          getCurrentCommand();
    PACKETCOMMAND_STATUS recv(byte* pkt, size_t len);                // Read packet header and locate a matching registered handler function
    PACKETCOMMAND_STATUS dispatch();
    byte*                getInputBuffer(){return _input_data;};
    int                  getInputBufferIndex();
    size_t               getInputLen(){return _input_len;};
    PACKETCOMMAND_STATUS setInputBufferIndex(int new_index);
    PACKETCOMMAND_STATUS moveInputBufferIndex(int n);
    byte*                getOutputBuffer(){return _output_data;};
    int                  getOutputBufferIndex();
    PACKETCOMMAND_STATUS setOutputBufferIndex(int new_index);
    PACKETCOMMAND_STATUS moveOutputBufferIndex(int n);
    //unpacking chars and bytes
    PACKETCOMMAND_STATUS unpack_byte(byte& varByRef);
    PACKETCOMMAND_STATUS unpack_byte_array(byte* buffer, int len);
    PACKETCOMMAND_STATUS unpack_char(char& varByRef);
    PACKETCOMMAND_STATUS unpack_char_array(char* buffer, int len);
    //unpacking stdint types
    PACKETCOMMAND_STATUS unpack_int8(    int8_t& varByRef);
    PACKETCOMMAND_STATUS unpack_uint8(  uint8_t& varByRef);
    PACKETCOMMAND_STATUS unpack_int16(  int16_t& varByRef);
    PACKETCOMMAND_STATUS unpack_uint16(uint16_t& varByRef);
    PACKETCOMMAND_STATUS unpack_int32(  int32_t& varByRef);
    PACKETCOMMAND_STATUS unpack_uint32(uint32_t& varByRef);
    PACKETCOMMAND_STATUS unpack_int64(  int64_t& varByRef);
    PACKETCOMMAND_STATUS unpack_uint64(uint64_t& varByRef);
    //unpacking floating point types
    PACKETCOMMAND_STATUS unpack_float(        float& varByRef);
    PACKETCOMMAND_STATUS unpack_double(      double& varByRef);
    PACKETCOMMAND_STATUS unpack_float32(  float32_t& varByRef);
    PACKETCOMMAND_STATUS unpack_float64(  float64_t& varByRef);
    //Methods for constructing an output
    PACKETCOMMAND_STATUS setupOutputCommandByName(const char* name);
    //packing chars and bytes
    PACKETCOMMAND_STATUS pack_byte(byte value);
    PACKETCOMMAND_STATUS pack_byte_array(byte* buffer, int len);
    PACKETCOMMAND_STATUS pack_char(char value);
    PACKETCOMMAND_STATUS pack_char_array(char* buffer, int len);
    //packing stdint types
    PACKETCOMMAND_STATUS pack_int8(    int8_t value);
    PACKETCOMMAND_STATUS pack_uint8(  uint8_t value);
    PACKETCOMMAND_STATUS pack_int16(  int16_t value);
    PACKETCOMMAND_STATUS pack_uint16(uint16_t value);
    PACKETCOMMAND_STATUS pack_int32(  int32_t value);
    PACKETCOMMAND_STATUS pack_uint32(uint32_t value);
    PACKETCOMMAND_STATUS pack_int64(  int64_t value);
    PACKETCOMMAND_STATUS pack_uint64(uint64_t value);
    //packing floating point types
    PACKETCOMMAND_STATUS pack_float(        float value);
    PACKETCOMMAND_STATUS pack_double(      double value);
    PACKETCOMMAND_STATUS pack_float32(  float32_t value);
    PACKETCOMMAND_STATUS pack_float64(  float64_t value);
    // Use the '_write_callback' to send return packet if registered
    PACKETCOMMAND_STATUS send();


  private:                                
    CommandInfo *_commandList;    //array to hold command entries
    CommandInfo _current_command; //command ready to dispatch
    CommandInfo _default_command; //called when a packet's Type ID is not recognized
    int     _commandCount;
    int     _maxCommands;
    //track state of input buffer
    byte*   _input_data;
    int     _input_index;
    size_t  _input_len;
    //track state of output buffer
    size_t  _outputBufferSize;
    byte*   _output_data;
    int     _output_index;
    size_t  _output_len;
    //use this callback to send out a packet
    void (*_write_callback)(byte* pkt, size_t len);                             //handler callback function

};

#endif //PACKETCOMMAND_H
