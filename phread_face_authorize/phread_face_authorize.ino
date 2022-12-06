#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

//======================
#include <WiFIMulti.h>
WiFiMulti WiFiMulti; 
//======================

#include "esp_camera.h"
#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"

/*
 * @author Gareth Rice
 * @version 11/15/22
 * 
 * This code should read image from ESP32 camera and send 
 * them to the server. Should also turn on the light if motion
 * is detected and it's dark out. 
 * 
 * To test, it will be opened with RFID, (if I can get that to work)
 * Should send ID over the network to be checked against IDs in a 
 * database on the server side. Right now, just checks it against 
 * hardcoded ID
 */

MFRC522DriverPinSimple ss_pin(5); 
MFRC522DriverSPI driver{ss_pin}; 
MFRC522 mfrc522{driver}; 

String tagContent = ""; 
String validID = "D2DE5F8D"; 

const char* ssid = "SSID";
const char* password = "password";
const String ip = "IP of Flask server"; //should be IP of server

const String endpoint = "http://" + ip + ":5000/auth/"; 

boolean motion = false; 
boolean dark = false; 
int switchPin = 33; 

camera_config_t config; 
void startCameraServer(); 
void config_init(); 


void setup() {
  Serial.begin(9600); 
//  WiFi.begin(ssid, password); 
//below was needed as a different way to connect to wifi when 
//ESP32 stopped working 
//============================================================
  WiFiMulti.addAP("SSID", "password");

    Serial.println();
    Serial.println();
    Serial.print("Waiting for WiFi... ");

    while(WiFiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    //================================================================

  pinMode(switchPin, INPUT); 

  mfrc522.PCD_Init(); 
//  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial); 
//  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks...")); 

  initCam(); 
}


void loop() {
//  if(motion && dark){
//    turnOnLight(); 
//  }

  //make sure still connected to WiFi network 
  if(WiFi.status() != WL_CONNECTED){
    wifiConnect(); 
  }

  //Authorize user through RFID
  if(authSwitch() == true){
    sendAuthorization(2); 
    //pause for 3 seconds so we don't make 1000 requests
    delay(3000); 
  }

  if(authRfid() == true){
    sendAuthorization(2); 
    delay(3000); 
  }

}


void wifiConnect(){
  while(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password); 
    delay(1000); 
    Serial.println("Connecting to WiFi..."); 
    WiFi.mode(WIFI_STA); 
  }
  Serial.println("Connected to the WiFi network");
}


boolean cardPresent(){
  boolean toReturn = true; 

  if(!mfrc522.PICC_IsNewCardPresent()){
    toReturn = false; 
  }
  if(!mfrc522.PICC_ReadCardSerial()){
    toReturn = false; 
  }

  return toReturn; 
}


boolean authRfid(){
  if(!cardPresent()){
    
    return false; 
  }

  tagContent = ""; 
  Serial.println("INSIDE AUTH RFID METHOD"); 

//  MFRC522Debug::PICC_DumpToSerial(mfrc522, Serial, &(mfrc522.uid));
//  Serial.println("Printing only the Card ID:");
//  Serial.println("--------------------------");
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    //tagContent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    tagContent.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  tagContent.toUpperCase();

  Serial.println(tagContent); //Print for finding new card
  Serial.println(tagContent.equals(validID)); 
  return tagContent.equals(validID); 
}


boolean authSwitch(){
  boolean toReturn = false;

  //use switch to test all the code and server 
  int switchVal = digitalRead(switchPin); 

  if(switchVal == 1){
    toReturn = true; 
  }else{
    toReturn = false; 
  }

  return toReturn; 
}


void turnOnLight(){
  
}


void sendAuthorization(int authCode){
  if(WiFi.status() != WL_CONNECTED){
    wifiConnect(); 
  }

  Serial.println("Sending auth"); 
  HTTPClient http; 
  http.begin(endpoint + String(authCode)); 
  int httpCode = http.GET(); 

  Serial.print("HTTP code: "); 
  Serial.println(httpCode); 
}


void config_init(){
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.fb_count = 1;
}


void initCam(){
  config_init(); 
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;  //range from 10 to 63. smaller number = sharper picture 

  //camera init
  esp_err_t err = esp_camera_init(&config); 
  if(err != ESP_OK){
    Serial.printf("Camera init failed with error 0x%x", err); 
    return; 
  }

  sensor_t *s = esp_camera_sensor_get(); 
  s->set_vflip(s, 1); //flip it back (0 no flip, 1 flip 180 deg)
  s->set_brightness(s, 1); //up the brightness a bit (-2 to 2 scale)
  s->set_saturation(s, -1); //lower the saturation (-2 to 2 scale)

  //can only call this after wifi has been init
  startCameraServer(); 
}
