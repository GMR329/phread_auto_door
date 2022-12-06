#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
  
/*
 * @author Gareth Rice
 * @version 11/15/22
 * 
 * This code should send a server request every so often and 
 * if it sees that we have been authorized, do everything 
 * involved with opening the door and closing it again 
 */

const char* ssid = "SSID";
const char* password = "password";
const String ip = "IP of Flask server"; //should be IP of server
 
const String endpoint = "http://" + ip + ":5000/go"; 

int servPin = 33; 
//int servLowPin = 32; 
int servKnob = 14; 

boolean authorized = false; 
boolean userInside = false; 
boolean doorOpen = false; 

Servo ser; 
//Servo serLow; 
Servo serKnob; 


void setup() {
  Serial.begin(115200); 
  ser.attach(servPin); 
  serKnob.attach(servKnob); 
//  serLow.attach(servLowPin); 
  WiFi.begin(ssid, password); 
}


void loop() {
  int auth = getAuth(); 
  
  //different auth codes needed for Python code
  //door should open as long as auth code > 0
  if(auth > 0){
//    if(!doorOpen){
      openDoor(); 
//      doorOpen = true; 
//      delay(5000); 
//      closeDoor(); 
//      doorOpen = false; 
//    }
  }
  delay(500); 
}


void wifiConnect(){
  while(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password); 
    WiFi.setSleep(false);  
    delay(1000); 
    Serial.println("Connecting to WiFi..."); 
  }
  Serial.println("Connected to the WiFi network");
}


int getAuth(){
  if(WiFi.status() != WL_CONNECTED){
    wifiConnect(); 
  }

  HTTPClient http; 
  http.begin(endpoint); 
  int httpCode = http.GET(); 
  int toReturn = 0; 

  if(httpCode > 0){
    String payload = http.getString(); 
    Serial.println(httpCode); 
    Serial.println(payload);
    //this does no check to make sure it's an int 
    toReturn = payload.toInt(); 
  }else{
    Serial.println("Error on HTTP request"); 
  }
  return toReturn; 
}


void openDoor(){
//  unlockDoor(); 
  turnKnob(); 
//  userInside = false; 

//  ser.write(0); 
////  serLow.write(0); 
//  delay(5000);
//
//  ser.write(75); 
//  delay(3000); 
}

void closeDoor(){
  //spin servo
  ser.write(75); 
//  serLow.write(75); 
  delay(3000);
}

void unlockDoor(){
  
}

void lockDoor(){
  
}

void turnKnob(){
  delay(1000);
  serKnob.write(180); 
  delay(5000); 
  serKnob.write(0); 
  delay(1000);
}

void releaseKnob(){
  
}
