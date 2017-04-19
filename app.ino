/*
 How to never run out of coffee with a connected coffee rack.
 By: Gildas Seimbille
 Thinxtra
 Date: April 19th, 2017
 License: Beerware license
 
 Special thanks: Nathan Seidle from SparkFun for the HX711 library.

 This example code uses the HX711 library: https://github.com/bogde/HX711
 And the XKit librairies: https://github.com/Thinxtra/Xkit-Sample
*/


#include <HX711.h>
#include <Isigfox.h>
#include <WISOL.h>
#include <Tsensors.h>
#include <Wire.h>
#include <math.h>
#include <SimpleTimer.h>

#define calibration_factor -7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define DOUT  9
#define CLK  10

Isigfox *Isigfox = new WISOL();
Tsensors *tSensors = new Tsensors();
SimpleTimer timer;
HX711 scale(DOUT, CLK);

typedef union{
    float number;
    uint8_t bytes[4];
} FLOATUNION_t;

typedef union{
    uint16_t number;
    uint8_t bytes[2];
} UINT16_t;

typedef union{
    int16_t number;
    uint8_t bytes[2];
} INT16_t;



void setup() {
  Wire.begin();
  Wire.setClock(100000);
  
  Serial.begin(9600);

  // WISOL test
  Isigfox->initSigfox();
  Isigfox->testComms();
  GetDeviceID();

  // Init sensors on Thinxtra Module
  tSensors->initSensors();
  tSensors->setReed(reedIR);
  tSensors->setButton(buttonIR); 
  
  Serial.println("Coffee Scale with a Kkit and a HX711");
  
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
  
  // Init timer to send a Sigfox message every 15 minutes
  unsigned long sendInterval = 900000;
  timer.setInterval(sendInterval, timeIR);
}

void loop() {
  timer.run();
}

void Send_Sensors(){
  UINT16_t tempt, photo, pressure, weight;
  char sendMsg[20];
  int sendlength;
  char sendstr[2];

  // Sending a float requires at least 4 bytes
  // In this demo, the measure values (temperature, pressure, sensor) are scaled to ranged from 0-65535.
  // Thus they can be stored in 2 bytes
  tempt.number = (uint16_t) (tSensors->getTemp() * 100);
  Serial.print("Temp: "); Serial.println((float)tempt.number/100);
  pressure.number =(uint16_t) (tSensors->getPressure()/3);
  Serial.print("Pressure: "); Serial.println((float)pressure.number*3);
  photo.number = (uint16_t) (tSensors->getPhoto() * 1000);
  Serial.print("Photo: "); Serial.println((float)photo.number/1000);
  weight.number = (uint16_t) (scale.get_units() * 1000);
  Serial.print("Scale: "); Serial.println((float)weight.number/1000);

    int payloadSize = 8; //in byte
//  byte* buf_str = (byte*) malloc (payloadSize);
  uint8_t buf_str[payloadSize];
  
  buf_str[0] = tempt.bytes[0];
  buf_str[1] = tempt.bytes[1];  
  buf_str[2] = pressure.bytes[0];
  buf_str[3] = pressure.bytes[1];
  buf_str[4] = photo.bytes[0];
  buf_str[5] = photo.bytes[1];
  buf_str[6] = weight.bytes[0];
  buf_str[7] = weight.bytes[1];

  Send_Pload(buf_str, payloadSize);
}

void reedIR(){
  Serial.println("Reed");
  timer.setTimeout(20, Send_Sensors); // send a Sigfox message after get ou IRS
}

void buttonIR(){
  Serial.println("Button");
  timer.setTimeout(20, Send_Sensors); // send a Sigfox message after get ou IRS
}

void timeIR(){
  Serial.println("Time");
  Send_Sensors();
}

void Send_Pload(uint8_t *sendData, int len){
  // No downlink message require
  recvMsg *RecvMsg;
  
  RecvMsg = (recvMsg *)malloc(sizeof(recvMsg));
  Isigfox->sendPayload(sendData, len, 0, RecvMsg);
  for (int i=0; i<RecvMsg->len; i++){
    Serial.print(RecvMsg->inData[i]);
  }
  Serial.println("");
  free(RecvMsg);
}


void GetDeviceID(){
  int headerLen = 6;
  recvMsg *RecvMsg;
  char msg[] = "AT$I=10";
  int msgLen = 7;
  
  RecvMsg = (recvMsg *)malloc(sizeof(recvMsg));
  Isigfox->sendMessage(msg, msgLen, RecvMsg);

  Serial.print("Device ID: ");
  for (int i=0; i<RecvMsg->len; i++){
    Serial.print(RecvMsg->inData[i]);
  }
  Serial.println("");
  free(RecvMsg);
}
