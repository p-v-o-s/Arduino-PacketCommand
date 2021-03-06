// Craig Versek, June 2015
#include <stdint.h>
#include <SerialCommand.h>



#include <PacketCommand.h>
#include <PacketQueue.h>

#define arduinoLED 13   // Arduino LED on board
#define SC_MAX_COMMANDS 20
#define PC_MAX_COMMANDS 20
#define PQ_CAPACITY 3

typedef float  float32_t;
typedef double float64_t;


void print_hex(byte* pkt, size_t len){
  Serial.print("0x");
  for (size_t i=0; i<len;i++){
    if(pkt[i] < 16){Serial.print(0x00,HEX);}
    Serial.print(pkt[i],HEX);
    Serial.print(" ");
  }
  Serial.println();
}

//------------------------------------------------------------------------------
SerialCommand sCmd(Serial,SC_MAX_COMMANDS);         // The demo PacketCommand object, initialize with any Stream object

PacketCommand pCmd(PC_MAX_COMMANDS);
PacketQueue pQ(PQ_CAPACITY);



//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off

  Serial.begin(9600);
  delay(1000);

  // Setup callbacks
  sCmd.setDefaultHandler(UNRECOGNIZED_sCmd_default_handler);
  sCmd.addCommand("LED.ON",     LED_ON_sCmd_action_handler);            // Turns LED on
  sCmd.addCommand("LED.OFF",    LED_OFF_sCmd_action_handler);           // Turns LED off
  
  // Testing functions for the PacketCommand object
  sCmd.addCommand("PCMD.RESET",  PCMD_RESET_sCmd_action_handler); //reset queue state
  //sCmd.addCommand("PCMD.ADDCMD", PCMD_ADDCMD_sCmd_action_handler); //reset queue state
  // Testing functions for the PacketQueue object
  sCmd.addCommand("PQ.SIZE?", PQ_SIZE_sCmd_query_handler); //reset queue state
  sCmd.addCommand("PQ.RESET", PQ_RESET_sCmd_action_handler); //reset queue state
  sCmd.addCommand("PQ.ENQ", PQ_ENQ_sCmd_action_handler);     //enqueue a string
  sCmd.addCommand("PQ.DEQ", PQ_DEQ_sCmd_action_handler);     //dequeue packet
  sCmd.addCommand("PQ.REQ", PQ_REQ_sCmd_action_handler);     //requeue a string
  
/*  //prepare a test packet*/
/*  test_pkt.data = (byte*) calloc(PQ_DATA_BUFFER_SIZE,sizeof(byte));*/
/*  test_pkt.length = 5;*/
/*  test_pkt.flags  = PacketQueue::PFLAG_IS_QUERY;*/
/*  for(int i=0; i < test_pkt.length;i++){*/
/*    test_pkt.data[i] = i+1;*/
/*  }*/
}

void loop() {
  sCmd.readSerial();
}

void LED_ON_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("LED_ON_sCmd_action_handlerLED on"));
  digitalWrite(arduinoLED, HIGH);
}

void LED_OFF_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("LED_OFF_sCmd_action_handlerLED off"));
  digitalWrite(arduinoLED, LOW);
}

void PCMD_RESET_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("---"));
  this_sCmd.println(F("cmd: PCMD_RESET_sCmd_action_handler"));
  char *arg = this_sCmd.next();
  if (arg == NULL){
    PacketShared::STATUS pcs;
    pcs = pCmd.reset();
    this_sCmd.print(F("pcs: "));this_sCmd.println(pcs);
    this_sCmd.println();
  }
  else{
    this_sCmd.print(F("### Error: PCMD.RESET requires no arguments\n"));
  }
  this_sCmd.println(F("..."));
}

void PCMD_ADDCMD_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("---"));
  this_sCmd.println(F("cmd: PCMD_ADDCMD_sCmd_action_handler"));
  char *arg1 = this_sCmd.next();
  if (arg1 == NULL){
    this_sCmd.print(F("### Error: PCMD.ADDCMD requires 2 arguments (str type_id, str name), 0 given\n"));
    return;
  }
  char *arg2 = this_sCmd.next();
  if (arg2 == NULL){
    this_sCmd.print(F("### Error: PCMD.ADDCMD requires 2 arguments (str type_id, str name), 1 given\n"));
    return;
  }
  this_sCmd.print(F("arg1: "));this_sCmd.println(arg1);
  this_sCmd.print(F("arg2: "));this_sCmd.println(arg2);
  PacketShared::STATUS pcs;
  pcs = pCmd.addCommand((byte*) arg1, arg2);
  this_sCmd.print(F("pcs: "));this_sCmd.println(pcs);
  this_sCmd.println(F("..."));
}


void PQ_SIZE_sCmd_query_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("---"));
  this_sCmd.println(F("cmd: PQ_SIZE_sCmd_query_handler"));
  char *arg = this_sCmd.next();
  if (arg == NULL){
    size_t size = pQ.size();
    this_sCmd.print(F("size: "));this_sCmd.println(size);
    this_sCmd.println();
  }
  else{
    this_sCmd.print(F("### Error: PQ.RESET requires no arguments\n"));
  }
  this_sCmd.println(F("..."));
}

void PQ_RESET_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("---"));
  this_sCmd.println(F("cmd: PQ_RESET_sCmd_action_handler"));
  char *arg = this_sCmd.next();
  if (arg == NULL){
    PacketShared::STATUS pqs;
    pqs = pQ.reset();
    this_sCmd.print(F("pqs: "));this_sCmd.println(pqs);
    this_sCmd.println();
  }
  else{
    this_sCmd.print(F("### Error: PQ.RESET requires no arguments\n"));
  }
  this_sCmd.println(F("..."));
}


void PQ_ENQ_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("---"));
  this_sCmd.println(F("cmd: PQ_ENQ_sCmd_action_handler"));
  char *arg = this_sCmd.next();
  if (arg == NULL){
    this_sCmd.print(F("### Error: PQ.ENQ requires 1 argument (str data)\n"));
  }
  else{
    size_t len = min(strlen(arg), PacketShared::DATA_BUFFER_SIZE);
    this_sCmd.print(F("arg: "));this_sCmd.println(arg);
    this_sCmd.print(F("len: "));this_sCmd.println(len);
    PacketShared::STATUS pqs;
    PacketShared::Packet pkt;
    pkt.length = len;
    //pkt.data   = (byte*) arg;
    memcpy(pkt.data,arg,len);
    pqs = pQ.enqueue(pkt);
    this_sCmd.print(F("pqs: "));this_sCmd.println(pqs);
  }
  this_sCmd.println(F("..."));
}

void PQ_DEQ_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("---"));
  this_sCmd.println(F("cmd: PQ_DEQ_sCmd_action_handler"));
  char *arg = this_sCmd.next();
  if (arg == NULL){
    PacketShared::STATUS pqs;
    PacketShared::Packet pkt;
    pqs = pQ.dequeue(pkt);
    this_sCmd.print(F("pqs: "));this_sCmd.println(pqs);
    this_sCmd.print(F("pkt.length: "));this_sCmd.println(pkt.length);
    this_sCmd.print(F("pkt.data: \""));
    for(size_t i=0; i < pkt.length; i++){
      this_sCmd.print((char) pkt.data[i]);
    }
    this_sCmd.println("\"");
  }
  else{
    this_sCmd.print(F("### Error: PQ.DEQ requires no arguments\n"));
  }
  this_sCmd.println(F("..."));
}

void PQ_REQ_sCmd_action_handler(SerialCommand this_sCmd) {
  this_sCmd.println(F("---"));
  this_sCmd.println(F("cmd: PQ_REQ_sCmd_action_handler"));
  char *arg = this_sCmd.next();
  if (arg == NULL){
    this_sCmd.print(F("### Error: PQ.REQ requires 1 argument (str data)\n"));
  }
  else{
    size_t len = min(strlen(arg), PacketShared::DATA_BUFFER_SIZE);
    this_sCmd.print(F("arg: "));this_sCmd.println(arg);
    this_sCmd.print(F("len: "));this_sCmd.println(len);
    PacketShared::STATUS pqs;
    PacketShared::Packet pkt;
    pkt.length = len;
    //pkt.data   = (byte*) arg;
    memcpy(pkt.data,arg,len);
    pqs = pQ.requeue(pkt);
    this_sCmd.print(F("pqs: "));this_sCmd.println(pqs);
  }
  this_sCmd.println(F("..."));
}



// Unrecognized command
void UNRECOGNIZED_sCmd_default_handler(const char* command, SerialCommand this_sCmd){
  this_sCmd.print(F("### Error: command '"));
  this_sCmd.print(command);
  this_sCmd.print(F("' not recognized ###\n"));
}

