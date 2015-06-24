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
    ERROR_QUEUE_UNDERFLOW        = -10,
    ERROR_MEMALLOC_FAIL          = -11
  } STATUS;

  static const size_t DATA_BUFFER_SIZE = 32;
  // Packet structure
  struct Packet {
    byte     data[DATA_BUFFER_SIZE];
    size_t   length;
    uint32_t timestamp;
    byte     flags;
  };
  
  enum InputPacketFlags {
       IPFLAG_IS_QUERY = 0x01,
       //IPFLAG_1 = 0x02,
       //IPFLAG_2 = 0x04,
       //IPFLAG_3 = 0x08,
       //IPFLAG_4 = 0x10,
       //IPFLAG_5 = 0x20,
       //IPFLAG_6 = 0x40,
       //IPFLAG_7 = 0x80
  };

  enum OutputPacketFlags {
       OPFLAG_IS_QUERY = 0x01,
       OPFLAG_APPEND_SEND_TIMESTAMP = 0x02,
       //OPFLAG_2 = 0x04,
       //OPFLAG_3 = 0x08,
       //OPFLAG_4 = 0x10,
       //OPFLAG_5 = 0x20,
       //OPFLAG_6 = 0x40,
       //OPFLAG_7 = 0x80
  };
};

#endif /* _PACKET_SHARED_H_INCLUDED */
