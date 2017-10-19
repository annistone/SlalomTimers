#include <stdio.h>
int i = 0;

#define IN_PROGRESS_IN 4
#define MEASURING_IN_PROGRESS_OUT 13

int  thisReceivedFlag = 0;
int laserMaxWorkingRate = 800;

struct Sensor{
  int pin_sensorIn;
  int pin_riderFlagOut;
  int pin_laserFlagLedOut;


  String sensorName;
  unsigned long timer_ms; 
  int sensorValue; 
  int prevSensorValue;
  int preprevSensorValue;
  int laserDiffValue;
  int laserValue;
};

struct Sensor sensorLeft  = {0, 2, 10, "Left", 0, 0, 0, 0, 200};
struct Sensor sensorRight = {1, 3, 11, "Right", 0, 0, 0, 0, 200};

void sensorSetup(Sensor sensor) {
  pinMode(sensor.pin_sensorIn, INPUT);
  pinMode(sensor.pin_riderFlagOut, OUTPUT);
  pinMode(sensor.pin_laserFlagLedOut, OUTPUT);

  digitalWrite(sensor.pin_riderFlagOut, 1);
  digitalWrite(sensor.pin_laserFlagLedOut, 1);
}

void setup() {
  Serial.begin(9600);
  sensorSetup(sensorLeft);
  sensorSetup(sensorRight);
  pinMode(IN_PROGRESS_IN, INPUT);
  pinMode(MEASURING_IN_PROGRESS_OUT, OUTPUT);
  digitalWrite(MEASURING_IN_PROGRESS_OUT, 0);
  
  delay(1000);
}

void sensorLoop(Sensor * sensor) {
  sensor->preprevSensorValue = sensor->prevSensorValue;
  sensor->prevSensorValue = sensor->sensorValue;
  sensor->sensorValue = analogRead(sensor->pin_sensorIn);
  if(((sensor->sensorValue - sensor->prevSensorValue) > sensor->laserDiffValue) && ((sensor->sensorValue - sensor->preprevSensorValue)> sensor->laserDiffValue)){ 
    sensor->timer_ms = millis();
    digitalWrite(sensor->pin_riderFlagOut, 1);
    digitalWrite(sensor->pin_laserFlagLedOut, 1);

    Serial.println();
    Serial.println();
    Serial.println(sensor->sensorName);
    char buffer [50];
    i=sprintf (buffer, "%i, %i, %i;",
                                          sensor->sensorValue, 
                                          sensor->prevSensorValue, 
                                          sensor->preprevSensorValue);
    for(int l= 0; l<=i; l++) 
     Serial.print(buffer[l]);
  }
     
  if((millis()- sensor->timer_ms) > 500){ 
    digitalWrite(sensor->pin_laserFlagLedOut, 0);
  }
  
  if (sensor->laserValue > laserMaxWorkingRate){
    if((millis()- sensor->timer_ms) > 1000){ 
      sensor->timer_ms = millis();
      digitalWrite(sensor->pin_laserFlagLedOut, 1);
    }
  }

  sensor->laserValue = (sensor->sensorValue + sensor->preprevSensorValue + sensor->prevSensorValue) / 3 ;
}

void loop() {
  if(digitalRead(IN_PROGRESS_IN)){ 
    digitalWrite(MEASURING_IN_PROGRESS_OUT, 1);
  }else{
    digitalWrite(MEASURING_IN_PROGRESS_OUT, 0);
    digitalWrite(sensorLeft.pin_riderFlagOut, 0);
    digitalWrite(sensorRight.pin_riderFlagOut, 0);
  }
  Serial.println();
  Serial.println(sensorLeft.sensorValue);
  Serial.println(sensorRight.sensorValue);
  
  sensorLoop(&sensorLeft);
  sensorLoop(&sensorRight);
}
