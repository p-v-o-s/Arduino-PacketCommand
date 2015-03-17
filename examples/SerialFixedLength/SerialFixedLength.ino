// Demo Code for PacketCommand Library using fixed length Serial packets
// Craig Versek, March 2015

#include <PacketCommand.h>
#include <stdint.h>
typedef float  float32_t;
typedef double float64_t;

#define arduinoLED 13   // Arduino LED on board
#define MAX_COMMANDS 10
#define PACKET_SIZE 10

PacketCommand pCmd(MAX_COMMANDS);         // The demo PacketCommand object, initialize with any Stream object

byte packetBuffer[PACKET_SIZE];  //a place to put incoming data, for example from a Serial stream or from a packet radio
int  packetBuffer_index = 0;

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
  pCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
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
    packetBuffer[packetBuffer_index] = inChar;
    packetBuffer_index++;
    if (packetBuffer_index >= PACKET_SIZE){             //we have a whole packet
      pcs = pCmd.readBuffer(packetBuffer, PACKET_SIZE); // send the packet to the PacketCommand parser
      if (pcs != SUCCESS){
        Serial.print(F("Error: pCmd.readBuffer returned status code: "));
        Serial.println(pcs);
      }
      else { //successful match or default, call the handler
        Serial.print("Got Packet: ");
        for(int i=0; i < PACKET_SIZE; i++){
            if(packetBuffer[i] <= 0x0F){
                Serial.print("0");
            }
            Serial.print(packetBuffer[i], HEX);
        }
        Serial.println();
        pCmd.dispatch();
      }
      //reset packet buffer index
      packetBuffer_index = 0;
    }
  }
  //do other stuff
}

void LED_on(PacketCommand this_pcmd) {
  Serial.println(F("LED on"));
  digitalWrite(arduinoLED, HIGH);
}

void LED_off(PacketCommand this_pcmd) {
  Serial.println(F("LED off"));
  digitalWrite(arduinoLED, LOW);
}

void handle_int(PacketCommand this_pcmd) {
  PACKETCOMMAND_STATUS pcs;
  int myInt = 0;
  pcs = this_pcmd.unpack_int32((int32_t&) myInt);
  if (pcs == SUCCESS){
    Serial.print(F("Got integer: "));
    Serial.println(myInt);
  }
  else{
    Serial.print(F("Error: this_pcmd.unpack_int32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_float(PacketCommand this_pcmd) {
  PACKETCOMMAND_STATUS pcs;
  float myFloat = 0;
  pcs = this_pcmd.unpack_float32((float32_t&) myFloat);
  if (pcs == SUCCESS){
    Serial.print(F("Got float: "));
    Serial.println(myFloat);
  }
  else{
    Serial.print(F("Error: this_pcmd.unpack_float32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_int_float(PacketCommand this_pcmd) {
  PACKETCOMMAND_STATUS pcs;
  int     myInt = 0;
  float myFloat = 0;
  pcs = this_pcmd.unpack_int32((int32_t&) myInt);
  if (pcs == SUCCESS){
    Serial.print(F("Got integer: "));
    Serial.println(myInt);
  }
  else{
    Serial.print(F("Error: this_pcmd.unpack_int32 returned status code: "));
    Serial.println(pcs);
  }
  pcs = this_pcmd.unpack_float32((float32_t&) myFloat);
  if (pcs == SUCCESS){
    Serial.print(F("Got float: "));
    Serial.println(myFloat);
  }
  else{
    Serial.print(F("Error: this_pcmd.unpack_float32 returned status code: "));
    Serial.println(pcs);
  }
}

void handle_char_array(PacketCommand this_pcmd) {
  PACKETCOMMAND_STATUS pcs;
  const int len = 9;
  char buffer[len + 1] = {0}; //remember to leave at least one element for zero termination of cstrings
  pcs = this_pcmd.unpack_char_array(buffer, len);
  if (pcs == SUCCESS){
    Serial.print(F("Got cstring: "));
    Serial.println(buffer);
  }
  else{
    Serial.print(F("Error: this_pcmd.unpack_char_array returned status code: "));
    Serial.println(pcs);
  }
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(PacketCommand this_pcmd) {
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
