// Demo Code for PacketCommand Library using fixed length Serial packets
// Craig Versek, March 2015

#include <PacketCommand.h>
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
void write_packet(byte* pkt, size_t len){
  //simulate a packet output by printing it in a hexidecimal format
  Serial.print("Sent Packet: ");
  print_hex(pkt,len);
}

//------------------------------------------------------------------------------

byte inputBuffer[PACKET_SIZE];  //a place to put incoming data, for example from a Serial stream or from a packet radio
int  inputBuffer_index = 0;
//------------------------------------------------------------------------------

void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off

  Serial.begin(9600);
  //delay(5000);

  // Setup callbacks for PacketCommand commands
  pCmd.addCommand((byte*) "\x41","LED.ON",     LED_on);            // Turns LED on   ("\x41" == "A")
  pCmd.addCommand((byte*) "\x42","LED.OFF",    LED_off);           // Turns LED off  ("\x42" == "B")
  pCmd.addCommand((byte*) "\x43","INT",        handle_int);        // unpacks 4-bytes and converts to int32_t ("\x43" == "C")
  pCmd.addCommand((byte*) "\x44","FLOAT",      handle_float);      // unpacks 4-bytes and converts to float   ("\x44" == "D")
  pCmd.addCommand((byte*) "\x45","CHAR_ARRAY", handle_char_array); // transfers N bytes into char[]   ("\x44" == "E")
  pCmd.addCommand((byte*) "\xFF\x01","INT_FLOAT", handle_int_float);  // unpacks 4-bytes (int32), 4-bytes (float)
  pCmd.addCommand((byte*) "\x46","WRITE_BACK_STUFF", handle_write_back_stuff); //("\x46" == "F")
  pCmd.registerDefaultHandler(unrecognized);                          // Handler for command that isn't matched  (says "What?")
  pCmd.registerWriteCallback(write_packet);
  //Serial.println("Ready");
}


void loop() {
  PACKETCOMMAND_STATUS pcs;
  char inChar;
  //handle incoming packets
  while (Serial.available() > 0) {
    inChar = Serial.read();   // Read single available character and place into buffer, there may be more waiting
    //Serial.print("Got char: ");
    //Serial.println(inChar);
    inputBuffer[inputBuffer_index] = inChar;
    inputBuffer_index++;
    if (inputBuffer_index >= PACKET_SIZE){             //we have a whole packet
      pcs = pCmd.readInputBuffer(inputBuffer, PACKET_SIZE); // send the packet to the PacketCommand parser
      if (pcs != SUCCESS){
        Serial.print(F("Error: pCmd.readBuffer returned status code: "));
        Serial.println(pcs);
      }
      else { //successful match or default, call the handler
        Serial.print("Got Packet: ");
        print_hex(inputBuffer, PACKET_SIZE);
        pCmd.dispatch();
      }
      //reset packet buffer index
      inputBuffer_index = 0;
    }
  }
  //do other stuff
}

void LED_on(PacketCommand this_pCmd) {
  Serial.println(F("LED on"));
  digitalWrite(arduinoLED, HIGH);
  this_pCmd.pack_byte((byte) 'Q');
  Serial.print(F("_output_index="));
  Serial.println(this_pCmd.getOutputBufferIndex());
  this_pCmd.writeOutputBuffer();
}

void LED_off(PacketCommand this_pCmd) {
  Serial.println(F("LED off"));
  digitalWrite(arduinoLED, LOW);
}

void handle_int(PacketCommand this_pCmd) {
  PACKETCOMMAND_STATUS pcs;
  int myInt = 0;
  pcs = this_pCmd.unpack_int32((int32_t&) myInt);
  if (pcs == SUCCESS){
    Serial.print(F("Got integer: "));
    Serial.println(myInt);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_int32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_float(PacketCommand this_pCmd) {
  PACKETCOMMAND_STATUS pcs;
  float myFloat = 0;
  pcs = this_pCmd.unpack_float32((float32_t&) myFloat);
  if (pcs == SUCCESS){
    Serial.print(F("Got float: "));
    Serial.println(myFloat);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_float32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_int_float(PacketCommand this_pCmd) {
  PACKETCOMMAND_STATUS pcs;
  int     myInt = 0;
  float myFloat = 0;
  pcs = this_pCmd.unpack_int32((int32_t&) myInt);
  if (pcs == SUCCESS){
    Serial.print(F("Got integer: "));
    Serial.println(myInt);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_int32 returned status code: "));
    Serial.println(pcs);
  }
  pcs = this_pCmd.unpack_float32((float32_t&) myFloat);
  if (pcs == SUCCESS){
    Serial.print(F("Got float: "));
    Serial.println(myFloat);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_float32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_char_array(PacketCommand this_pCmd) {
  PACKETCOMMAND_STATUS pcs;
  const int len = 9;
  char buffer[len + 1] = {0}; //remember to leave at least one element for zero termination of cstrings
  pcs = this_pCmd.unpack_char_array(buffer, len);
  if (pcs == SUCCESS){
    Serial.print(F("Got cstring: "));
    Serial.println(buffer);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_char_array returned status code: "));
    Serial.println(pcs);
  }
}

void handle_write_back_stuff(PacketCommand this_pCmd) {
  this_pCmd.lookupCommandByName("LED.ON");
  PacketCommand::CommandInfo cmd = this_pCmd.getCurrentCommand(); //should be the LED.ON command type_id="\x41"
  this_pCmd.pack_byte(cmd.type_id[0]);
  this_pCmd.pack_int32(987654321);
  this_pCmd.pack_float32(3.141592);
  this_pCmd.pack_char_array("hello",5);
  this_pCmd.writeOutputBuffer();
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(PacketCommand this_pCmd) {
  PacketCommand::CommandInfo current_command;
  Serial.print(F("Did not recognize \""));
  Serial.print(F("type_id="));
  for(int i=0; i < MAX_TYPE_ID_LEN; i++){
    if( current_command.type_id[i] != 0x00){
        Serial.print(current_command.type_id[i], HEX);
    }
  }
  Serial.println(F("\" as a command."));
}
