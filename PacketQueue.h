/*  
*/
#ifndef _PACKET_QUEUE_H_INCLUDED
#define _PACKET_QUEUE_H_INCLUDED

#include <stdint.h>

#include "PacketShared.h"

//uncomment for debugging
//#define PACKETQUEUE_DEBUG

#ifdef PACKETQUEUE_DEBUG
  #ifdef DEBUG_PORT
    #define PACKETQUEUE_DEBUG_PORT DEBUG_PORT
  #else
    #define PACKETQUEUE_DEBUG_PORT Serial
  #endif
#endif

class PacketQueue
{
public:
  PacketQueue();
  PacketShared::STATUS begin(size_t capacity);
  PacketShared::STATUS end();
  size_t size() const { return _size; }
  size_t capacity() const { return _capacity; }
  PacketShared::STATUS reset();
  PacketShared::STATUS enqueue(PacketShared::Packet& pkt);
  PacketShared::STATUS dequeue(PacketShared::Packet& pkt);
  PacketShared::STATUS requeue(PacketShared::Packet& pkt);
  // Empty the queue,return number of packets flushed
  size_t flush();

private:
  void _put_at(size_t index, PacketShared::Packet& pkt);
  void _get_from(size_t index, PacketShared::Packet& pkt);
  
  volatile size_t _beg_index;
  volatile size_t _end_index;
  volatile size_t _size;
  size_t _capacity;
  size_t _dataBufferSize;
  PacketShared::Packet* _slots;
  
};

#endif /* _PACKET_QUEUE_H_INCLUDED */
