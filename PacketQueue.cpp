/*  PacketQueue

*/
#include <Arduino.h>
#include "PacketQueue.h"

PacketQueue::PacketQueue()
  : _beg_index(0)
  , _end_index(0)
  , _size(0)
  , _capacity(0)
  , _dataBufferSize(PacketShared::DATA_BUFFER_SIZE)
{
//  //preallocate memory for all the slots
//  _slots = (PacketShared::Packet*) calloc(_capacity, sizeof(PacketShared::Packet));
//  PacketShared::Packet *pkt_slot;
//  for(size_t i=0; i < _capacity; i++){
//    pkt_slot = &(_slots[i]); //pull out the slot by address
//    //pkt_slot->data = (byte*) calloc(_dataBufferSize, sizeof(byte));
//    pkt_slot->length = 0;
//    pkt_slot->flags  = 0x00;
//  }
}

PacketShared::STATUS PacketQueue::begin(size_t capacity)
{
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println(F("# In PacketQueue::begin"));
  #endif
  //preallocate memory for all the slots
  _slots = (PacketShared::Packet*) calloc(capacity, sizeof(PacketShared::Packet));
  if (_slots == NULL){
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.println("### Error failed to allocate memory for the queue!");
    #endif
    return PacketShared::ERROR_MEMALLOC_FAIL;
  } 
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.print("# \tallocated &_slots=");DEBUG_PORT.println((int) &_slots, HEX);
  #endif
  PacketShared::Packet *pkt_slot;
  for(size_t i=0; i < _capacity; i++){
    pkt_slot = &(_slots[i]); //pull out the slot by address
    //pkt_slot->data = (byte*) calloc(_dataBufferSize, sizeof(byte));
    pkt_slot->length = 0;
    pkt_slot->flags  = 0x00;
  }
  _capacity = capacity;  //make sure to cache
  return PacketShared::SUCCESS;
}

PacketShared::STATUS PacketQueue::end()
{
  //PacketShared::Packet *pkt_slot;
  //for(size_t i=0; i < _capacity; i++){
  //  pkt_slot = &(_slots[i]); //pull out the slot by address
  //  //free(pkt_slot->data);
  //}
  free(_slots);
  return PacketShared::SUCCESS;
}

PacketShared::STATUS PacketQueue::reset()
{
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println(F("# In PacketQueue::reset"));
  #endif
  _size= 0;
  _beg_index = 0;
  _end_index = 0;
  return PacketShared::SUCCESS;
}

PacketShared::STATUS PacketQueue::enqueue(PacketShared::Packet& pkt)
{
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println(F("# In PacketQueue::enqueue"));
  #endif
  if ((_size + 1) <= _capacity){
    _put_at(_end_index, pkt);
     //adjust the size and indices
    _size++;
    _end_index = (_end_index + 1) % _capacity; //wrap around if needed
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.println(F("# (enqueue) after copy"));
    DEBUG_PORT.print(F("# \t_end_index="));DEBUG_PORT.println(_end_index);
    DEBUG_PORT.print(F("# \t_size="));DEBUG_PORT.println(_size);
    #endif
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.println(F("\t### Error: Queue Overflow"));
    #endif
    return PacketShared::ERROR_QUEUE_OVERFLOW;
  }
}

PacketShared::STATUS PacketQueue::dequeue(PacketShared::Packet& pkt)
{
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println(F("# In PacketQueue::dequeue"));
  #endif
  if (_size > 0){
    _get_from(_beg_index, pkt);
    //adjust the size and indices
    _beg_index = (_beg_index + 1) % _capacity; //wrap around if needed
    _size--;
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.println(F("# (dequeue) after copy"));
    DEBUG_PORT.print(F("# \t_beg_index="));DEBUG_PORT.println(_beg_index);
    DEBUG_PORT.print(F("# \t_size="));DEBUG_PORT.println(_size);
    #endif
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.println(F("### Error: Queue Underflow"));
    #endif
    pkt.length = 0; //set to safe value
    return PacketShared::ERROR_QUEUE_UNDERFLOW;
  }
}

PacketShared::STATUS PacketQueue::requeue(PacketShared::Packet& pkt)
{
  //pushes packet onto the front of the queue
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println(F("# In PacketCommand::requeue"));
  #endif
  if ((_size + 1) <= _capacity){
    //update the size and indices ahead of time
    _size++;
    _beg_index = (_beg_index == 0)? (_capacity - 1) : (_beg_index - 1); //wrap around if needed
    _put_at(_beg_index, pkt);
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.println(F("# (requeue) after copy"));
    DEBUG_PORT.print(F("# \t_beg_index="));DEBUG_PORT.println(_beg_index);
    DEBUG_PORT.print(F("# \t_size="));DEBUG_PORT.println(_size);
    #endif
    return PacketShared::SUCCESS;
  }
  else{
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.println(F("### Error: Queue Overflow"));
    #endif
    return PacketShared::ERROR_QUEUE_OVERFLOW;
  }
}

void PacketQueue::_put_at(size_t index, PacketShared::Packet& pkt)
{
  PacketShared::Packet *pkt_slot = &(_slots[index]); //pull out the slot by address
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println(F("# In PacketQueue::_put_at"));
  DEBUG_PORT.print(F("# \tindex="));DEBUG_PORT.println(index);
  DEBUG_PORT.print(F("# \t&pkt="));DEBUG_PORT.println((unsigned int) &pkt,HEX);
  DEBUG_PORT.print(F("# \tpkt_slot="));DEBUG_PORT.println((unsigned int) pkt_slot,HEX);
  DEBUG_PORT.print(F("# \tcopying data: "));
  #endif
  //copy the packet object into the slot
  for(size_t i=0; i < pkt.length; i++){
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.print(pkt.data[i], HEX);DEBUG_PORT.print(F(" "));
    #endif
    pkt_slot->data[i] = pkt.data[i];
  }
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println();
  #endif
  pkt_slot->length    = pkt.length; //update length field
  pkt_slot->timestamp = pkt.timestamp; //update length field
  pkt_slot->flags  = pkt.flags;
}

void PacketQueue::_get_from(size_t index, PacketShared::Packet& pkt)
{
  PacketShared::Packet *pkt_slot = &(_slots[index]); //pull out the slot by address
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println(F("# In PacketQueue::_get_from"));
  DEBUG_PORT.print(F("# \tindex="));DEBUG_PORT.println(index);
  DEBUG_PORT.print(F("# \t&pkt="));DEBUG_PORT.println((unsigned int) &pkt,HEX);
  DEBUG_PORT.print(F("# \tpkt_slot="));DEBUG_PORT.println((unsigned int) pkt_slot,HEX);
  DEBUG_PORT.print(F("# \tpkt_slot->length="));DEBUG_PORT.println(pkt_slot->length);
  DEBUG_PORT.print(F("# \tcopying data: 0x"));
  #endif
  //copy the slot data to the current the referenced packet object
  for(size_t i=0; (i < pkt_slot->length) && (i < _dataBufferSize); i++){
    #ifdef PACKETQUEUE_DEBUG
    DEBUG_PORT.print(pkt_slot->data[i], HEX);DEBUG_PORT.print(F(" "));
    #endif
    pkt.data[i] = pkt_slot->data[i];
  }
  #ifdef PACKETQUEUE_DEBUG
  DEBUG_PORT.println();
  #endif
  pkt.length    = min(pkt_slot->length,_dataBufferSize);
  pkt.timestamp = pkt_slot->timestamp;
  pkt.flags     = pkt_slot->flags;
}
