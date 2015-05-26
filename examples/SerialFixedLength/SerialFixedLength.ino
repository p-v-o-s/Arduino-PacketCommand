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

bool serial_recv_callback(PacketCommand this_pCmd){
  static byte inputBuffer[PACKET_SIZE];  //a place to put incoming data, for example from a Serial stream or from a packet radio
  static int  inputBuffer_index = 0;
  //handle incoming packets
  while (Serial.available() > 0) {
    char inChar = Serial.read();   // Read single available character and place into buffer, there may be more waiting
    //Serial.print("Got char: ");
    //Serial.println(inChar);
    inputBuffer[inputBuffer_index] = inChar;
    inputBuffer_index++;
    if (inputBuffer_index >= PACKET_SIZE){ //we have a whole packet
      this_pCmd.assignInputBuffer(inputBuffer, PACKET_SIZE);
      inputBuffer_index = 0;
      return true;
    }
  }
  return false;
}

void serial_send_callback(PacketCommand this_pCmd){
  //simulate a packet output by printing it in a hexidecimal format
  byte* outbuf = this_pCmd.getOutputBuffer();
  size_t   len = this_pCmd.getOutputLen();
  Serial.print("Sent Packet: ");
  print_hex(outbuf,len);
}


//------------------------------------------------------------------------------


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
  pCmd.registerRecvCallback(serial_recv_callback);
  pCmd.registerSendCallback(serial_send_callback);
  //Serial.println("Ready");
}


void loop() {
  PacketCommand::STATUS pcs;
  
  //do other stuff
}

void LED_on(PacketCommand this_pCmd) {
  Serial.println(F("LED on"));
  digitalWrite(arduinoLED, HIGH);
}

void LED_off(PacketCommand this_pCmd) {
  Serial.println(F("LED off"));
  digitalWrite(arduinoLED, LOW);
}

void handle_int(PacketCommand this_pCmd) {
  PacketCommand::STATUS pcs;
  int myInt = 0;
  pcs = this_pCmd.unpack_int32((int32_t&) myInt);
  if (pcs == PacketCommand::SUCCESS){
    Serial.print(F("Got integer: "));
    Serial.println(myInt);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_int32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_float(PacketCommand this_pCmd) {
  PacketCommand::STATUS pcs;
  float myFloat = 0;
  pcs = this_pCmd.unpack_float32((float32_t&) myFloat);
  if (pcs == PacketCommand::SUCCESS){
    Serial.print(F("Got float: "));
    Serial.println(myFloat);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_float32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_int_float(PacketCommand this_pCmd) {
  PacketCommand::STATUS pcs;
  int     myInt = 0;
  float myFloat = 0;
  pcs = this_pCmd.unpack_int32((int32_t&) myInt);
  if (pcs == PacketCommand::SUCCESS){
    Serial.print(F("Got integer: "));
    Serial.println(myInt);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_int32 returned status code: "));
    Serial.println(pcs);
  }
  pcs = this_pCmd.unpack_float32((float32_t&) myFloat);
  if (pcs == PacketCommand::SUCCESS){
    Serial.print(F("Got float: "));
    Serial.println(myFloat);
  }
  else{
    Serial.print(F("Error: this_pCmd.unpack_float32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_char_array(PacketCommand this_pCmd) {
  PacketCommand::STATUS pcs;
  const int len = 9;
  char buffer[len + 1] = {0}; //remember to leave at least one element for zero termination of cstrings
  pcs = this_pCmd.unpack_char_array(buffer, len);
  if (pcs == PacketCommand::SUCCESS){
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
  this_pCmd.send();
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(PacketCommand this_pCmd) {
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
