// Demo Code for SerialCommand Library
// Craig Versek, Jan 2014
// based on code from Steven Cogswell, May 2011

#include <SerialCommand.h>

#define arduinoLED 13   // Arduino LED on board

SerialCommand sCmd(Serial);         // The demo SerialCommand object, initialize with any Stream object

void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off

  Serial.begin(9600);

  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("ON",    LED_on);          // Turns LED on
  sCmd.addCommand("OFF",   LED_off);         // Turns LED off
  sCmd.addCommand("HELLO", sayHello);        // Echos the string argument back
  sCmd.addCommand("P",     processCommand);  // Converts two arguments to integers and echos them back
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  Serial.println("Ready");
}

void loop() {
  sCmd.readSerial();     // We don't do much, just process serial commands
}


void LED_on(SerialCommand this_scmd) {
  this_scmd.println("LED on");
  digitalWrite(arduinoLED, HIGH);
}

void LED_off(SerialCommand this_scmd) {
  this_scmd.println("LED off");
  digitalWrite(arduinoLED, LOW);
}

void sayHello(SerialCommand this_scmd) {
  char *arg;
  arg = this_scmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL) {    // As long as it existed, take it
    this_scmd.print("Hello ");
    this_scmd.println(arg);
  }
  else {
    this_scmd.println("Hello, whoever you are");
  }
}


void processCommand(SerialCommand this_scmd) {
  int aNumber;
  char *arg;

  this_scmd.println("We're in processCommand");
  arg = this_scmd.next();
  if (arg != NULL) {
    aNumber = atoi(arg);    // Converts a char string to an integer
    this_scmd.print("First argument was: ");
    this_scmd.println(aNumber);
  }
  else {
    this_scmd.println("No arguments");
  }

  arg = this_scmd.next();
  if (arg != NULL) {
    aNumber = atol(arg);
    this_scmd.print("Second argument was: ");
    this_scmd.println(aNumber);
  }
  else {
    this_scmd.println("No second argument");
  }
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command, SerialCommand this_scmd) {
  this_scmd.print("Did not recognize \"");
  this_scmd.print(command);
  this_scmd.println("\" as a command.");
}
