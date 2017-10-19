#include <Ticker.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Adafruit_NeoPixel.h>
#include "SLALOM_display.h" 

#define SENSOR_LEFT_IN     12
#define SENSOR_RIGHT_IN    14
#define RECEIVED_OUT       16

#define NEOPIXEL_OUT       13
#define NUMPIXELS           8
#define RED_DELAY        1000
#define GREEN_DELAY      3000
#define BETWEEN_DELAY    1000


#define EspSensorReady      1
#define EspStartRace        4
#define EspSemaphoreStarted 5
#define EspLeftTriggered    6
#define EspRightTriggered   7
#define EspSensorStart      1
#define EspSensorFinish     2
#define EspPing             8
#define EspPong             9

const char* ssid     = "SlalomRaces2017";
const char* password = "SLALOMslalom2017";
const char* host = "192.168.137.1";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_OUT, NEO_GRB + NEO_KHZ800);
WebSocketsClient ws;  
Ticker riderTimer;
Adafruit_PCD8544 display = Adafruit_PCD8544(15, 2, 0, 4, 5);

bool sensorReceivedFlag_left = false;
unsigned long sensorValue_left = 0;

bool sensorReceivedFlag_right = false;
unsigned long sensorValue_right = 0;

bool sensorSendedFlag_left = true;
bool sensorSendedFlag_right = true;

bool timersReceivedFlag = false;

unsigned long t_left = 0;
unsigned long t_right = 0;
unsigned long st = 0;

unsigned long espTime = 0;
unsigned long startTime = 0;

bool wsIsConnected = false;

void setup() {
  delay(7000);

  pinMode(SENSOR_LEFT_IN, INPUT);
  pinMode(RECEIVED_OUT, OUTPUT);
  pinMode(SENSOR_RIGHT_IN, INPUT);
  digitalWrite(RECEIVED_OUT, 0);
  delay(1000);
  
  Serial.begin(9600);
  Serial.printf("Serial begin.");
  displaySetup();
  displayNewString("Connecting to wifi...");
  Serial.printf("\nConnecting to wifi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  
  WiFi.setAutoReconnect(true);
  ws.begin(host, 1234, "/esp");
  ws.onEvent(wsHandler);
  pixels.begin();
  delay(1000);
}

void initialize(){
    StaticJsonBuffer<100> jsonBuffer1;
    JsonObject& jsonInit = jsonBuffer1.createObject();
    char buff[100];
    
    jsonInit["Command"] = EspSensorReady;
    jsonInit["Name"] = "StartSensor"; 
    jsonInit["Type"] = EspSensorStart;
    jsonInit.printTo(buff);
    ws.sendTXT(buff);
    delay(10);
}

void startMeasurings(){

  sensorReceivedFlag_left = false;
  sensorReceivedFlag_right = false;
  
  riderTimer.detach();
  displayNewString("Measuring..");

  digitalWrite(RECEIVED_OUT, HIGH);
  
  riderTimer.attach_ms(2, sensorTimerIsr);
  neopixelsSignal();
}

void wsHandler(WStype_t type, uint8_t * payload, size_t length) {
 switch(type) {
    case WStype_ERROR:
      displayAddString("WS error!");
      Serial.printf("\nWs error!");
      break;
    case WStype_CONNECTED:
      if(wsIsConnected == false){
          riderTimer.detach();
          displayNewString("Server connected.");
          initialize();    
          displayAddString("Initialized.");      
          riderTimer.attach_ms(2000, displayUpdate);
      }
      wsIsConnected = true;
      Serial.printf("\nWs connected");
      break;
    case WStype_DISCONNECTED:
      if(wsIsConnected == true){
         displayAddString("Server disconnected.");
      }
      wsIsConnected = false;
      Serial.printf("\nWs not connected");
      break;
    case WStype_TEXT:
      Serial.printf("\nReceived message.");
     
      Serial.printf("\n%s", payload);
      Serial.printf("\n%i", length);
      
      StaticJsonBuffer<100> jsonBuffer1;
      JsonObject& jsonCommand = jsonBuffer1.parseObject(payload);
      
      if (jsonCommand["Command"] == EspStartRace){ 
        Serial.printf("\nStarting measurings.");
        espTime == millis();
        startMeasurings();
      }
        
      if (jsonCommand["Command"] == EspPing){
          Serial.printf("\nReceived ping.");
          
          char buff[100];         
          jsonCommand["Command"] = EspPong;
          jsonCommand.printTo(buff);
          ws.sendTXT(buff);
          Serial.printf("\nSend pong.");
          delay(10);
      }
      break;
    }
}

void neopixelsSignal(){
  for(int j=0;j<3;j++){
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(255,0,0));
    }
    pixels.show();
    delay(RED_DELAY);
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0));
    }
    pixels.show();
    if(j != 2)
      delay(BETWEEN_DELAY);
    else
      delay(random(1000,5000));
  }

  for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,255,0));
    }
    pixels.show();
    
    startTime = millis(); 
    
    StaticJsonBuffer<50> jsonBuffer2;
    JsonObject& jsonSemaphore = jsonBuffer2.createObject();
    char buff[50];
    jsonSemaphore["Command"] = EspSemaphoreStarted;
    jsonSemaphore["Time"] = startTime - espTime; 
    jsonSemaphore.printTo(buff);
    ws.sendTXT(buff);
    
    displayAddString("ST sended");
    
    delay(GREEN_DELAY);
    for(int i=0;i<NUMPIXELS;i++){
      pixels.setPixelColor(i, pixels.Color(0,0,0));
    }
    pixels.show();
}

void sensorTimerIsr(){ 
  if(digitalRead(SENSOR_LEFT_IN) && (!sensorReceivedFlag_left)){
    sensorValue_left = millis();
    sensorReceivedFlag_left = true; 
    sensorSendedFlag_left = false;
  }
  
  if(digitalRead(SENSOR_RIGHT_IN) && (!sensorReceivedFlag_right)){
    sensorValue_right = millis();
    sensorReceivedFlag_right = true;
    sensorSendedFlag_right = false;
  }
}

void loop() {
  ws.loop();
  if(!sensorSendedFlag_left){
    t_left = sensorValue_left - espTime;
    StaticJsonBuffer<100> jsonBuffer2;
    JsonObject& jsonTimers = jsonBuffer2.createObject();
    char buff[50];
    jsonTimers["Command"] = EspLeftTriggered;
    jsonTimers["Time"] = t_left; 
    jsonTimers.printTo(buff);
    ws.sendTXT(buff);
    sensorSendedFlag_left = true;
    displayAddString("LT sended");
  }
  
  if(!sensorSendedFlag_right){
    t_right = sensorValue_right - espTime;
    StaticJsonBuffer<100> jsonBuffer2;
    JsonObject& jsonTimers = jsonBuffer2.createObject();
    char buff[50];
    jsonTimers["Command"] = EspRightTriggered;
    jsonTimers["Time"] = t_right; 
    jsonTimers.printTo(buff);
    ws.sendTXT(buff);
    sensorSendedFlag_right = true;
    displayAddString("RT sended");
  }
  
  if(sensorReceivedFlag_left && sensorReceivedFlag_right){
    digitalWrite(RECEIVED_OUT, LOW);
    riderTimer.detach();
    riderTimer.attach_ms(2000, displayUpdate);
    sensorReceivedFlag_left = false;
    sensorReceivedFlag_right = false;
  }
}
