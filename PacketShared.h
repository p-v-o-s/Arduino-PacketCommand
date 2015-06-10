/*  
*/
#ifndef _PACKET_SHARED_H_INCLUDED
#define _PACKET_SHARED_H_INCLUDED

#include <stdint.h>


namespace PacketShared{
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
    ERROR_INPUT_BUFFER_OVERRUN   = -8,
    ERROR_QUEUE_OVERFLOW         = -9,
    ERROR_QUEUE_UNDERFLOW        = -10
  } STATUS;

  static const size_t DATA_BUFFER_SIZE = 32;
  // Packet structure
  struct Packet {
    byte    data[DATA_BUFFER_SIZE];
    size_t  length;
    byte    flags;
  };

  enum PacketFlags {
       PFLAG_IS_QUERY = 0x01,
       //PFLAG_1 = 0x02,
       //PFLAG_2 = 0x04,
       //PFLAG_3 = 0x08,
       //PFLAG_4 = 0x10,
       //PFLAG_5 = 0x20,
       //PFLAG_6 = 0x40,
       //PFLAG_7 = 0x80
  };
};

#endif /* _PACKET_SHARED_H_INCLUDED */
