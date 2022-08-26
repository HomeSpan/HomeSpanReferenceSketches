/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2022 Gregg E. Berman
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

#include "HomeSpan.h"

///////////////////////////////

struct Humidifier : Service::HumidifierDehumidifier
{

  SpanCharacteristic *active;
  SpanCharacteristic *currentTemperature;
  SpanCharacteristic *targetTemperature;
  SpanCharacteristic *currentHeatingCoolingState;
  SpanCharacteristic *targetHeatingCoolingState;
  SpanCharacteristic *coolingThresholdTemperature;
  SpanCharacteristic *heatingThresholdTemperature;
  // SpanCharacteristic *rotationSpeed;
  boolean updating = false;

  ThermostatController() : Service::Thermostat()
  {
    // 0 - off
    // 1 - on
    this->active = new Characteristic::Active();

    // value in centigrade
    this->currentTemperature = new Characteristic::CurrentTemperature(10.0);
    this->targetTemperature = new Characteristic::TargetTemperature(10.0);

    // 0 - inactive
    // 1 - idle
    // 2 - heating
    // 3 - cooling
    this->currentHeatingCoolingState = new Characteristic::CurrentHeatingCoolingState();
    this->targetHeatingCoolingState = new Characteristic::TargetHeatingCoolingState();

    this->coolingThresholdTemperature = new Characteristic::CoolingThresholdTemperature();
    this->heatingThresholdTemperature = new Characteristic::HeatingThresholdTemperature();

    new Characteristic::TemperatureDisplayUnits();

    // value between 0 and 100
    // this->rotationSpeed = new Characteristic::RotationSpeed();

  }
};

struct Thermo : Service::Thermostat {
   
  boolean update() override {
      
    return(true);                              // return true 
  }

};
      
///////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.begin(Category::Bridges,"HomeSpan Tests");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Humidifier");

    new Service::HumidifierDehumidifier();
      new Characteristic::Active();   
      new Characteristic::CurrentRelativeHumidity();
      new Characteristic::CurrentHumidifierDehumidifierState();
      new Characteristic::TargetHumidifierDehumidifierState();
      new Characteristic::RelativeHumidityDehumidifierThreshold();
      new Characteristic::RelativeHumidityHumidifierThreshold();
      new Characteristic::RotationSpeed();
      new Characteristic::SwingMode();
      new Characteristic::WaterLevel();
      new Characteristic::LockPhysicalControls();
      

//    new Service::Fan();
//      new Characteristic::Active();   
//      new Characteristic::CurrentFanState();    
//      new Characteristic::TargetFanState();
//      new Characteristic::RotationDirection();
//      new Characteristic::RotationSpeed();
//      new Characteristic::SwingMode();
//      new Characteristic::LockPhysicalControls();  
      
}

///////////////////////////////

void loop() {
  homeSpan.poll();
}

///////////////////////////////
