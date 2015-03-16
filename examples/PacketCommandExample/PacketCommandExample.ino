// Demo Code for PacketCommand Library
// Craig Versek, March 2015

#include <PacketCommand.h>

#define arduinoLED 13   // Arduino LED on board
#define MAX_COMMANDS 10
#define PACKET_SIZE 4

PacketCommand pCmd(MAX_COMMANDS);         // The demo PacketCommand object, initialize with any Stream object

char packetBuffer[PACKET_SIZE];  //a place to put incoming data, for example from a Serial stream or from a packet radio
int  packetBuffer_index = 0;

void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off

  Serial.begin(9600);
  delay(5000);

  // Setup callbacks for PacketCommand commands
  pCmd.addCommand("\x41","LED.ON",    LED_on);          // Turns LED on
  pCmd.addCommand("\xFF\x42","LED.OFF",   LED_off);         // Turns LED off
/*  pCmd.addCommand("\x03","HELLO", sayHello);        // Echos the string argument back*/
/*  pCmd.addCommand("\x04","P",     processCommand);  // Converts two arguments to integers and echos them back*/
/*  pCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")*/
  Serial.println("Ready");
}


void loop() {
  PACKETCOMMAND_STATUS pcs;
  char inChar;
  //handle incoming packets
  while (Serial.available() > 0) {
    inChar = Serial.read();   // Read single available character and place into buffer, there may be more waiting
    Serial.print("Got char: ");
    Serial.println(inChar);
    packetBuffer[packetBuffer_index] = inChar;
    packetBuffer_index++;
    if (packetBuffer_index >= PACKET_SIZE){             //we have a whole packet
      pcs = pCmd.readBuffer(packetBuffer, PACKET_SIZE); // send the packet to the PacketCommand parser
      if (pcs != SUCCESS){
        Serial.print("Error: pCmd.readBuffer return status code: ");
        Serial.println(pcs);
      }
      else { //successful match, call the handler
        pCmd.dispatch();
      }
      //reset packet buffer index
      packetBuffer_index = 0;
    }
  }
  //do other stuff
}

void LED_on(PacketCommand this_pcmd) {
  Serial.println("LED on");
  digitalWrite(arduinoLED, HIGH);
}

void LED_off(PacketCommand this_pcmd) {
  Serial.println("LED off");
  digitalWrite(arduinoLED, LOW);
}

/*void sayHello(PacketCommand this_pcmd) {*/
/*  char *arg;*/
/*  arg = this_pcmd.next();    // Get the next argument from the PacketCommand object buffer*/
/*  if (arg != NULL) {    // As long as it existed, take it*/
/*    this_pcmd.print("Hello ");*/
/*    this_pcmd.println(arg);*/
/*  }*/
/*  else {*/
/*    this_pcmd.println("Hello, whoever you are");*/
/*  }*/
/*}*/


/*void processCommand(PacketCommand this_pcmd) {*/
/*  int aNumber;*/
/*  char *arg;*/

/*  this_pcmd.println("We're in processCommand");*/
/*  arg = this_pcmd.next();*/
/*  if (arg != NULL) {*/
/*    aNumber = atoi(arg);    // Converts a char string to an integer*/
/*    this_pcmd.print("First argument was: ");*/
/*    this_pcmd.println(aNumber);*/
/*  }*/
/*  else {*/
/*    this_pcmd.println("No arguments");*/
/*  }*/

/*  arg = this_pcmd.next();*/
/*  if (arg != NULL) {*/
/*    aNumber = atol(arg);*/
/*    this_pcmd.print("Second argument was: ");*/
/*    this_pcmd.println(aNumber);*/
/*  }*/
/*  else {*/
/*    this_pcmd.println("No second argument");*/
/*  }*/
/*}*/

/*// This gets set as the default handler, and gets called when no other command matches.*/
/*void unrecognized(PacketCommand this_pcmd) {*/
/*  this_pcmd.print("Did not recognize \"");*/
/*  //this_pcmd.print(command);*/
/*  this_pcmd.println("\" as a command.");*/
/*}*/
