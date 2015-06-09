// Demo Code for PacketCommand Library using fixed length Serial packets
// Craig Versek, March 2015


#define PACKETCOMMAND_DEBUG
#include <PacketCommand.h>
#include <PacketQueue.h>
#include <stdint.h>
typedef float  float32_t;
typedef double float64_t;

#define arduinoLED 13   // Arduino LED on board
#define MAX_COMMANDS 10
#define PACKET_SIZE 10


void print_hex(byte* pkt, size_t len){
  Serial.print("0x");
  for (int i=0; i<len;i++){
    if(pkt[i] < 16){Serial.print(0x00,HEX);}
    Serial.print(pkt[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
}

//------------------------------------------------------------------------------
PacketCommand pCmd(MAX_COMMANDS);         // The demo PacketCommand object, initialize with any Stream object

#define PQ_CAPACITY 3
#define PQ_DATA_BUFFER_SIZE 32
PacketQueue pQ(PQ_CAPACITY,PQ_DATA_BUFFER_SIZE);

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off

  Serial.begin(9600);
  delay(5000);

  // Setup callbacks for PacketCommand commands
  pCmd.addCommand((byte*) "\x41","LED.ON",     LED_on);            // Turns LED on   ("\x41" == "A")
  pCmd.addCommand((byte*) "\x42","LED.OFF",    LED_off);           // Turns LED off  ("\x42" == "B")
  pCmd.registerDefaultHandler(unrecognized);                          // Handler for command that isn't matched  (says "What?")
  
  
/*  byte* input_buff = pCmd.getInputBuffer();*/
/*  pCmd.resetInputBuffer();*/
/*  for(int i=0; i < 5;i++){*/
/*    input_buff[i] = i+1;*/
/*  }*/
/*  pCmd.assignInputBuffer(input_buff,5);*/
/*  pCmd.enqueueInputBuffer();*/
/*  pCmd.resetInputBuffer();*/
/*  for(int i=0; i < 5;i++){*/
/*    input_buff[i] = i+6;*/
/*  }*/
/*  pCmd.assignInputBuffer(input_buff,5);*/
/*  pCmd.enqueueInputBuffer();*/
/*  pCmd.dequeueInputBuffer();*/
/*  print_hex(input_buff,5);*/
/*  pCmd.dequeueInputBuffer();*/
/*  print_hex(input_buff,5);*/
}


void loop() {
  PacketCommand::STATUS pcs;
  //pCmd.processInput();
  //do other stuff
  
  PacketQueue::STATUS pqs;
  
  //prepare a test packet
  PacketQueue::Packet test_pkt;
  test_pkt.data = (byte*) calloc(PQ_DATA_BUFFER_SIZE,sizeof(byte));
  test_pkt.length = 5;
  test_pkt.flags  = PacketQueue::PFLAG_IS_QUERY;
  for(int i=0; i < test_pkt.length;i++){
    test_pkt.data[i] = i+1;
  }
  
  //enqueue test
  Serial.println("--- testing enqueue:");
  pQ.enqueue(test_pkt);
  delay(1000);
  
  //dequeue test
  Serial.println("--- testing dequeue:");
  PacketQueue::Packet out_pkt;
  out_pkt.data = (byte*) calloc(PQ_DATA_BUFFER_SIZE,sizeof(byte));
  pQ.dequeue(out_pkt);
  Serial.print("    out_pkt.length=");Serial.print(out_pkt.length);Serial.println();
  Serial.print("    got back data:");
  print_hex(out_pkt.data,out_pkt.length);Serial.println();
  delay(1000);
  
  Serial.println("--- testing enqueue to fill up:");
  for (int i=0; i < PQ_CAPACITY; i++){
    pQ.enqueue(test_pkt);
    delay(1000);
  }
  
  //testing overflow
  Serial.println("--- enqueue to overflow:");
  pqs = pQ.enqueue(test_pkt);
    Serial.print("    pqs=");Serial.println(pqs);
  delay(1000);
  
  Serial.println("--- testing dequeue to empty:");
  for (int i=0; i < PQ_CAPACITY; i++){
    pQ.dequeue(out_pkt);
    Serial.print("    out_pkt.length=");Serial.print(out_pkt.length);Serial.println();
    Serial.print("    got back data:");
    print_hex(out_pkt.data,out_pkt.length);Serial.println();
    delay(1000);
  }
  
  delay(100000);
}

void LED_on(PacketCommand& this_pCmd) {
  Serial.println(F("LED on"));
  digitalWrite(arduinoLED, HIGH);
}

void LED_off(PacketCommand& this_pCmd) {
  Serial.println(F("LED off"));
  digitalWrite(arduinoLED, LOW);
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(PacketCommand& this_pCmd) {
  PacketCommand::CommandInfo current_command;
  Serial.print(F("Did not recognize \""));
  Serial.print(F("type_id="));
  for(int i=0; i < PacketCommand::MAX_TYPE_ID_LEN; i++){
    if( current_command.type_id[i] != 0x00){
        Serial.print(current_command.type_id[i], HEX);
    }
  }
  Serial.println(F("\" as a command."));
}
