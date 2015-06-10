/*  PacketQueue

*/
#include <Arduino.h>
#include "PacketQueue.h"

PacketQueue::PacketQueue(size_t capacity)
  : _beg_index(0)
  , _end_index(0)
  , _size(0)
  , _capacity(capacity)
  , _dataBufferSize(DATA_BUFFER_SIZE)
{
  //preallocate memory for all the slots
  _slots = (Packet*) calloc(_capacity, sizeof(Packet));
  Packet *pkt_slot;
  for(size_t i=0; i < _capacity; i++){
    pkt_slot = &(_slots[i]); //pull out the slot by address
    //pkt_slot->data = (byte*) calloc(_dataBufferSize, sizeof(byte));
    pkt_slot->length = 0;
    pkt_slot->flags  = 0x00;
  }
}

PacketQueue::~PacketQueue()
{
  Packet *pkt_slot;
  for(size_t i=0; i < _capacity; i++){
    pkt_slot = &(_slots[i]); //pull out the slot by address
    //free(pkt_slot->data);
  }
  free(_slots);
}

PacketQueue::STATUS PacketQueue::reset()
{
  #ifdef PACKETQUEUE_DEBUG
  Serial.println(F("# In PacketQueue::reset"));
  #endif
  _size= 0;
  _beg_index = 0;
  _end_index = 0;
  return SUCCESS;
}

PacketQueue::STATUS PacketQueue::enqueue(PacketQueue::Packet& pkt)
{
  #ifdef PACKETQUEUE_DEBUG
  Serial.println(F("# In PacketQueue::enqueue"));
  #endif
  if ((_size + 1) <= _capacity){
    _put_at(_end_index, pkt);
     //adjust the size and indices
    _size++;
    _end_index = (_end_index + 1) % _capacity; //wrap around if needed
    #ifdef PACKETQUEUE_DEBUG
    Serial.println(F("# (enqueue) after copy"));
    Serial.print(F("# \t_end_index="));Serial.println(_end_index);
    Serial.print(F("# \t_size="));Serial.println(_size);
    #endif
    return SUCCESS;
  }
  else{
    #ifdef PACKETQUEUE_DEBUG
    Serial.println(F("\t### Error: Queue Overflow"));
    #endif
    return ERROR_QUEUE_OVERFLOW;
  }
}

PacketQueue::STATUS PacketQueue::dequeue(PacketQueue::Packet& pkt)
{
  #ifdef PACKETQUEUE_DEBUG
  Serial.println(F("# In PacketQueue::dequeue"));
  #endif
  if (_size > 0){
    _get_from(_beg_index, pkt);
    //adjust the size and indices
    _beg_index = (_beg_index + 1) % _capacity; //wrap around if needed
    _size--;
    #ifdef PACKETQUEUE_DEBUG
    Serial.println(F("# (dequeue) after copy"));
    Serial.print(F("# \t_beg_index="));Serial.println(_beg_index);
    Serial.print(F("# \t_size="));Serial.println(_size);
    #endif
    return SUCCESS;
  }
  else{
    #ifdef PACKETQUEUE_DEBUG
    Serial.println(F("### Error: Queue Underflow"));
    #endif
    pkt.length = 0; //set to safe value
    return ERROR_QUEUE_UNDERFLOW;
  }
}

PacketQueue::STATUS PacketQueue::requeue(PacketQueue::Packet& pkt)
{
  //pushes packet onto the front of the queue
  #ifdef PACKETQUEUE_DEBUG
  Serial.println(F("# In PacketCommand::requeue"));
  #endif
  if ((_size + 1) <= _capacity){
    //update the size and indices ahead of time
    _size++;
    int d = _capacity;
    int i = (_beg_index - 1) % d;
    _beg_index = (i < 0)? i+d : i; //wrap around if needed
    _put_at(_beg_index, pkt);
    #ifdef PACKETQUEUE_DEBUG
    Serial.println(F("# (requeue) after copy"));
    Serial.print(F("# \t_beg_index="));Serial.println(_beg_index);
    Serial.print(F("# \t_size="));Serial.println(_size);
    #endif
    return SUCCESS;
  }
  else{
    #ifdef PACKETQUEUE_DEBUG
    Serial.println(F("### Error: Queue Overflow"));
    #endif
    return ERROR_QUEUE_OVERFLOW;
  }
}

void PacketQueue::_put_at(size_t index, PacketQueue::Packet& pkt)
{
  Packet *pkt_slot = &(_slots[index]); //pull out the slot by address
  #ifdef PACKETQUEUE_DEBUG
  Serial.println(F("# In PacketQueue::_put_at"));
  Serial.print(F("# \tindex="));Serial.println(index);
  Serial.print(F("# \t&pkt="));Serial.println((unsigned int) &pkt,HEX);
  Serial.print(F("# \tpkt_slot="));Serial.println((unsigned int) pkt_slot,HEX);
  Serial.print(F("# \tcopying data: "));
  #endif
  //copy the packet object into the slot
  for(int i=0; i < pkt.length; i++){
    #ifdef PACKETQUEUE_DEBUG
    Serial.print(pkt.data[i], HEX);Serial.print(F(" "));
    #endif
    pkt_slot->data[i] = pkt.data[i];
  }
  #ifdef PACKETQUEUE_DEBUG
  Serial.println();
  #endif
  pkt_slot->length = pkt.length; //update length field
  pkt_slot->flags  = pkt.flags;
}

void PacketQueue::_get_from(size_t index, PacketQueue::Packet& pkt)
{
  Packet *pkt_slot = &(_slots[index]); //pull out the slot by address
  #ifdef PACKETQUEUE_DEBUG
  Serial.println(F("# In PacketQueue::_get_from"));
  Serial.print(F("# \tindex="));Serial.println(index);
  Serial.print(F("# \t&pkt="));Serial.println((unsigned int) &pkt,HEX);
  Serial.print(F("# \tpkt_slot="));Serial.println((unsigned int) pkt_slot,HEX);
  Serial.print(F("# \tpkt_slot->length="));Serial.println(pkt_slot->length);
  Serial.print(F("# \tcopying data: 0x"));
  #endif
  //copy the slot data to the current the referenced packet object
  for(int i=0; (i < pkt_slot->length) && (i < _dataBufferSize); i++){
    #ifdef PACKETQUEUE_DEBUG
    Serial.print(pkt_slot->data[i], HEX);Serial.print(F(" "));
    #endif
    pkt.data[i] = pkt_slot->data[i];
  }
  #ifdef PACKETQUEUE_DEBUG
  Serial.println();
  #endif
  pkt.length = min(pkt_slot->length,_dataBufferSize);
  pkt.flags  = pkt_slot->flags;
}
