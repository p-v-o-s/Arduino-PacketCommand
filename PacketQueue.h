/*  
*/
#ifndef _PACKET_QUEUE_H_INCLUDED
#define _PACKET_QUEUE_H_INCLUDED

//uncomment for debugging
#define PACKETQUEUE_DEBUG

class PacketQueue
{
public:
  static const size_t DATA_BUFFER_SIZE = 32;
  // Status and Error  Codes
  typedef enum StatusCode {
    SUCCESS = 0,
    ERROR_QUEUE_OVERFLOW        = -1,
    ERROR_QUEUE_UNDERFLOW       = -2
  } STATUS;

  //Packet flags
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
  // Packet structure
  struct Packet {
    byte    data[DATA_BUFFER_SIZE];
    size_t  length;
    byte    flags;
  };

  PacketQueue(size_t capacity);
  ~PacketQueue();

  size_t size() const { return _size; }
  size_t capacity() const { return _capacity; }
  STATUS reset();
  STATUS enqueue(Packet& pkt);
  STATUS dequeue(Packet& pkt);
  STATUS requeue(Packet& pkt);
  // Empty the queue,return number of packets flushed
  size_t flush();

private:
  void _put_at(size_t index, Packet& pkt);
  void _get_from(size_t index, Packet& pkt);
  
  int _beg_index;
  int _end_index;
  size_t _size; 
  size_t _capacity;
  size_t _dataBufferSize;
  Packet* _slots;
  
};

#endif /* _PACKET_QUEUE_H_INCLUD */
