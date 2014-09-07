/*********************************************************************
This is an adaption of the original Bluetooth Low Energy Breakout code by Adafruit.
It's adapted to make use of my own design. 


Original header below:

This is an example for our nRF8001 Bluetooth Low Energy Breakout

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1697

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Kevin Townsend/KThttps://github.com/Rosthouse/vibrobag_arduino.gitOWN  for Adafruit Industries.
MIT license, check LICENSE for more information
All text above, and the splash screen below must be included in any redistribution
*********************************************************************/

// This version uses the internal data queing so you can treat it like Serial (kinda)!

#include <SPI.h>
#include "Adafruit_BLE_UART.h"

// Connect CLK/MISO/MOSI to hardware SPI
// e.g. On UNO & compatible: CLK = 13, MISO = 12, MOSI = 11
#define ADAFRUITBLE_REQ 10
#define ADAFRUITBLE_RDY 2     // This should be an interrupt pin, on Uno thats #2 or #3
#define ADAFRUITBLE_RST 9
#define SCREEN_SWITCH 7


Adafruit_BLE_UART BTLEserial = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);
/**************************************************************************/
/*!
    Configure the Arduino and start advertising with the radio
*/
/**************************************************************************/
void setup(void)
{ 
  Serial.begin(9600);
  while(!Serial); // Leonardo/Micro should wait for serial init
  Serial.println(F("VibroBag Starting up"));
  
  //Enabling vibrator
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  //defining pin 7 to enable the smartphone screen
  pinMode(7, INPUT);
  

  BTLEserial.begin();
  Serial.println(F("VibroBag ready to operate"));
}

/**************************************************************************/
/*!
    Constantly checks for new events on the nRF8001
*/
/**************************************************************************/
aci_evt_opcode_t laststatus = ACI_EVT_DISCONNECTED;

//Main loop
void loop(){
  aci_evt_opcode_t status = getStatus();
  checkPreviousState(status);
  if (status == ACI_EVT_CONNECTED) {
    // Lets see if there's any data for us!
    String commandString = receiveCommand();
    executeCommand(commandString);
    bool activateDisplay = enableConnectedDisplay();
    // Next up, see if we have any data to get from the Serial console
    if(activateDisplay){
      activateConnectedDisplay();
    }
    writeSerialInfos();
  }
}

//Gets the current status of the bluetooth breakout
aci_evt_opcode_t getStatus(){
  // Tell the nRF8001 to do whatever it should be working on.
  BTLEserial.pollACI();
  // Ask what is our current status
  return BTLEserial.getState();
}

//Checks if any change of state has occured. This is purely for logging
void checkPreviousState(aci_evt_opcode_t current){

  // If the status changed....
  if (current != laststatus) {
    // print it out!
    if (current == ACI_EVT_DEVICE_STARTED) {
        Serial.println(F("* Advertising started"));
    }
    if (current == ACI_EVT_CONNECTED) {
        Serial.println(F("* Connected!"));
    }
    if (current == ACI_EVT_DISCONNECTED) {
        Serial.println(F("* Disconnected or advertising timed out"));
    }
    // OK set the last status change to this one
    
    laststatus = current;
  }

}

//Checks if a commando has been sent to arduino
String receiveCommand(){
  char command[20];
  String commandString = String("");
  if (BTLEserial.available()) {
    Serial.print("* "); 
    Serial.print(BTLEserial.available()); 
    Serial.println(F(" bytes available from BTLE"));
    int availableBytes = BTLEserial.available();
    char command[availableBytes+1];
    for(int i=0; i<availableBytes; i++){
        command[i] = BTLEserial.read();
    }
    command[availableBytes] = '\0';
    Serial.println(command);
    commandString = String(command);
  }
  return commandString;
}

//Executes a commando sent to Arduino
void executeCommand(String command){      
  if(command.equals("vbon")){
    Serial.println("Activating Vibration");
    digitalWrite(4, HIGH); 
  } else if(command.equals("vboff")){
    Serial.println("Deactivating Vibration");
    digitalWrite(4, LOW); 
  } 
}

//Writes a string to a connected device. Max 20 chars long
void writeStringToConnectedDevice(String message){
  uint8_t sendbuffer[20];
  message.getBytes(sendbuffer, 20);
  char sendbuffersize = min(20, message.length());
  Serial.print(F("\n* Sending -> \"")); 
      Serial.print((char *)sendbuffer); 
      Serial.println("\"");
  // write the data
      BTLEserial.write(sendbuffer, sendbuffersize);
}

//Sends a message to the connected smartphone to activate its display
void activateConnectedDisplay(){
  String s = String ("dspon");
  writeStringToConnectedDevice(s);
}

//Writes information coming from serial to the connected device
void writeSerialInfos(){
  if (Serial.available()) {
      // Read a line from Serial
      Serial.setTimeout(100); // 100 millisecond timeout
      String s = Serial.readString();
      writeStringToConnectedDevice(s);
    }
}

//Checks wheter to activate the connected smartphones display
int previousSwitchState = LOW;
bool enableConnectedDisplay(){
  bool enableDisplay = false;
  int switchState = digitalRead( SCREEN_SWITCH );
  if( switchState != previousSwitchState ){
    if( switchState == HIGH ){
      Serial.println("Display will activate now");
      enableDisplay = true;
    }
  }
  previousSwitchState = switchState;
  return enableDisplay;
}