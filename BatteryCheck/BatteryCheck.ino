/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2023 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
///////////////////////////////////////////////////////
//                                                   //
//   HomeSpan Reference Sketch: Battery Service      //
//                                                   //
///////////////////////////////////////////////////////

#include "HomeSpan.h" 

////////////////////////////////////////////////////////////////////////
//                        BATTERY CLASS                               //
////////////////////////////////////////////////////////////////////////

// This standalone class (separate from HomeSpan) is designed to measure the battery voltage
// on an Adafruit ESP32 Huzzah Featherboard in which the LiPo is hardwired to pin 35 and
// battery voltage can be read by analogRead(35).

// These analog values range from about 1850 (battery just about drained) to over 2400 (battery fully charged).

// In addition, by connecting the USB voltage pin on the ESP32 to digital pin 21 through a voltage divider
// consisting of two 10K ohm resistors, it is possible to determine whether or not the ESP is plugged
// into USB power by calling digitalRead(21).  This can be used to determine whether or not the battery is
// being charged.

// The class supports the following two methods:
//
//  * int getPercentCharged() - returns 0-100
//  * int getChargingState() - returns 1 if ESP32 is plugged into USB power, else returns 0

// Note: battery voltage is automatically checked by this class in a background task that runs every second.

class BATTERY {
  
  int batteryPin;         // pin to use for an analog read of Battery Voltage
  int usbPin;             // pin to use for a digital read of USB Voltage
  int minReading;         // min expected analog value of Battery Voltage (corresponding to 0% charged)
  int maxReading;         // max expected analog value of Battery Voltage (corresponding to 100% charged)
  float analogReading;    // analog reading of the Battery Voltage

  public:

  BATTERY(int batteryPin, int usbPin, int minReading, int maxReading){
    this->batteryPin=batteryPin;
    this->usbPin=usbPin;
    this->minReading=minReading;
    this->maxReading=maxReading;
    analogReading=maxReading;
        
    pinMode(usbPin,INPUT_PULLDOWN);     // set usbPin to input mode
    
    xTaskCreateUniversal(batteryUpdate, "batteryTaskHandle", 4096, this, 1, NULL, 0);   // start background task to measure analogRead(35)
  }

  // returns percent charged from 0-100
  
  int getPercentCharged(){
    int percentCharged=100.0*(analogReading-minReading)/(maxReading-minReading);

    if(percentCharged>100)
      percentCharged=100;
    else if(percentCharged<0)
      percentCharged=0;

    return(percentCharged);
  }

  // returns 1 if USB is powered, else 0

  int getChargingState(){
    return(digitalRead(usbPin));
  }

  // background task that measures voltage of battery
  
  static void batteryUpdate(void *args){
    BATTERY *b = (BATTERY*)args;
    for(;;){
      b->analogReading*=0.9;
      b->analogReading+=0.1*analogRead(b->batteryPin);      // use exponential smoothing
      delay(1000);
    }
  }
  
};

BATTERY Battery(35,21,1850,2400);     // create a global instance of the BATTERY Class for use by the Battery Service below

////////////////////////////////////////////////////////////////////////
//                        HomeSpan Code                               //
////////////////////////////////////////////////////////////////////////

struct SimpleLED : Service::LightBulb {

  Characteristic::On power;
  int ledPin;
 
  SimpleLED(int ledPin) : Service::LightBulb(){
    this->ledPin=ledPin;
    pinMode(ledPin,OUTPUT);    
  }

  boolean update(){            
    digitalWrite(ledPin,power.getNewVal());
    return(true);
  }
};

////////////////////////////////////////////////////////////////////////

struct SimpleBattery : Service::BatteryService{

  SpanCharacteristic *percentCharged;
  SpanCharacteristic *chargingState;
  SpanCharacteristic *lowBattery;
  int lowPercent;

  SimpleBattery(int lowPercent) : Service::BatteryService(){

    this->lowPercent=lowPercent;

    percentCharged = new Characteristic::BatteryLevel(Battery.getPercentCharged());
    chargingState = new Characteristic::ChargingState(Battery.getChargingState());
    lowBattery = new Characteristic::StatusLowBattery(Battery.getPercentCharged()<lowPercent?1:0);
  }
  
  void loop() override {
        
    if(Battery.getChargingState()!=chargingState->getVal())          // update Charging State immediately if changed
      chargingState->setVal(Battery.getChargingState());

    if(percentCharged->timeVal()>5000 && Battery.getPercentCharged()!=percentCharged->getVal()){   // update Percent Charged only once every 5 seconds if changed
      percentCharged->setVal(Battery.getPercentCharged());
      lowBattery->setVal(Battery.getPercentCharged()<lowPercent?1:0);
    }
  }
};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  homeSpan.begin(Category::Lighting,"HomeSpan LED");
  
  new SpanAccessory(); 
    new Service::AccessoryInformation(); 
      new Characteristic::Identify();                
    new SimpleLED(13);                      // create a LightBulb Service operating a simple LED pin 13
    new SimpleBattery(20);                  // create a Battery Service using 20% as the threshold for a low-battery warning in the Home App
}

////////////////////////////////////////////////////////////////////////

void loop(){
  homeSpan.poll();  
}

////////////////////////////////////////////////////////////////////////
