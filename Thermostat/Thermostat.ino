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
//   HomeSpan Reference Sketch: Thermostat Service   //
//                                                   //
///////////////////////////////////////////////////////

#include "HomeSpan.h"

#define MIN_TEMP  0         // minimum allowed temperature in celsius
#define MAX_TEMP  40        // maximum allowed temperature in celsius

////////////////////////////////////////////////////////////////////////

// Here we create a dummmy temperature sensor that can be used as a real sensor in the Thermostat Service below.
// Rather than read a real temperature sensor, this structure allows you to change the current temperature via the Serial Monitor

struct DummyTempSensor {
  static float temp;
  
  DummyTempSensor(float t) {
    temp=t;
    new SpanUserCommand('f',"<temp> - set the temperature, where temp is in degrees F", [](const char *buf){temp=(atof(buf+1)-32.0)/1.8;});
    new SpanUserCommand('c',"<temp> - set the temperature, where temp is in degrees C", [](const char *buf){temp=atof(buf+1);});
  }

  float read() {return(temp);}
};

float DummyTempSensor::temp;

////////////////////////////////////////////////////////////////////////

struct Reference_Thermostat : Service::Thermostat {

  // Create characteristics, set initial values, and set storage in NVS to true

  Characteristic::CurrentHeatingCoolingState currentState{0,true};
  Characteristic::TargetHeatingCoolingState targetState{0,true}; 
  Characteristic::CurrentTemperature currentTemp{22,true};
  Characteristic::TargetTemperature targetTemp{22,true};
  Characteristic::CurrentRelativeHumidity currentHumidity{50,true};
  Characteristic::TargetRelativeHumidity targetHumidity{50,true};
  Characteristic::HeatingThresholdTemperature heatingThreshold{22,true};
  Characteristic::CoolingThresholdTemperature coolingThreshold{22,true};  
  Characteristic::TemperatureDisplayUnits displayUnits{0,true};               // this is for changing the display on the actual thermostat (if any), NOT in the Home App

  DummyTempSensor tempSensor{22};                                             // instantiate a dummy temperature sensor with initial temp=22 degrees C
 
  Reference_Thermostat() : Service::Thermostat() {
    Serial.printf("\n*** Creating HomeSpan Thermostat***\n");

    currentTemp.setRange(MIN_TEMP,MAX_TEMP);                                  // set all ranges the same to make sure Home App displays them correctly on the same dial
    targetTemp.setRange(MIN_TEMP,MAX_TEMP);
    heatingThreshold.setRange(MIN_TEMP,MAX_TEMP);
    coolingThreshold.setRange(MIN_TEMP,MAX_TEMP);    
  }

  boolean update() override {

    if(targetState.updated()){
      switch(targetState.getNewVal()){
        case 0:
          Serial.printf("Thermostat turning OFF\n");
          break;
        case 1:
          Serial.printf("Thermostat set to HEAT at %s\n",temp2String(targetTemp.getVal<float>()).c_str());
          break;
        case 2:
          Serial.printf("Thermostat set to COOL at %s\n",temp2String(targetTemp.getVal<float>()).c_str());
          break;
        case 3:
          Serial.printf("Thermostat set to AUTO from %s to %s\n",temp2String(heatingThreshold.getVal<float>()).c_str(),temp2String(coolingThreshold.getVal<float>()).c_str());
          break;
      }
    }

    if(heatingThreshold.updated() || coolingThreshold.updated())
      Serial.printf("Temperature range changed to %s to %s\n",temp2String(heatingThreshold.getNewVal<float>()).c_str(),temp2String(coolingThreshold.getNewVal<float>()).c_str());
      
    else if(targetTemp.updated())
      Serial.printf("Temperature target changed to %s\n",temp2String(targetTemp.getNewVal<float>()).c_str());

    if(displayUnits.updated())
      Serial.printf("Display Units changed to %c\n",displayUnits.getNewVal()?'F':'C');

    if(targetHumidity.updated())
      Serial.printf("Humidity target changed to %d%%\n",targetHumidity.getNewVal());
    
    return(true);
  }

  // Here's where all the main logic exists to turn on/off heating/cooling by comparing the current temperature to the Thermostat's settings

  void loop() override {

      float temp=tempSensor.read();       // read temperature sensor (which in this example is just a dummy sensor)
      
      if(temp<MIN_TEMP)                   // limit value to stay between MIN_TEMP and MAX_TEMP
        temp=MIN_TEMP;
      if(temp>MAX_TEMP)
        temp=MAX_TEMP;

      if(currentTemp.timeVal()>5000 && fabs(currentTemp.getVal<float>()-temp)>0.25){      // if it's been more than 5 seconds since last update, and temperature has changed
        currentTemp.setVal(temp);                                                       
        Serial.printf("Current Temperature is now %s.\n",temp2String(currentTemp.getNewVal<float>()).c_str());
      } 

      switch(targetState.getVal()){
        
        case 0:
          if(currentState.getVal()!=0){
            Serial.printf("Thermostat OFF\n");
            currentState.setVal(0);
          }
          break;
          
        case 1:
          if(currentTemp.getVal<float>()<targetTemp.getVal<float>() && currentState.getVal()!=1){
            Serial.printf("Turning HEAT ON\n");
            currentState.setVal(1);
          }
          else if(currentTemp.getVal<float>()>=targetTemp.getVal<float>() && currentState.getVal()==1){
            Serial.printf("Turning HEAT OFF\n");
            currentState.setVal(0);
          }
          else if(currentState.getVal()==2){
            Serial.printf("Turning COOL OFF\n");
            currentState.setVal(0);            
          }
          break;
          
        case 2:
          if(currentTemp.getVal<float>()>targetTemp.getVal<float>() && currentState.getVal()!=2){
            Serial.printf("Turning COOL ON\n");
            currentState.setVal(2);
          }
          else if(currentTemp.getVal<float>()<=targetTemp.getVal<float>() && currentState.getVal()==2){
            Serial.printf("Turning COOL OFF\n");
            currentState.setVal(0);
          }
          else if(currentState.getVal()==1){
            Serial.printf("Turning HEAT OFF\n");
            currentState.setVal(0);            
          }
          break;
          
        case 3:
          if(currentTemp.getVal<float>()<heatingThreshold.getVal<float>() && currentState.getVal()!=1){
            Serial.printf("Turning HEAT ON\n");
            currentState.setVal(1);
          }
          else if(currentTemp.getVal<float>()>=heatingThreshold.getVal<float>() && currentState.getVal()==1){
            Serial.printf("Turning HEAT OFF\n");
            currentState.setVal(0);
          }
          
          if(currentTemp.getVal<float>()>coolingThreshold.getVal<float>() && currentState.getVal()!=2){
            Serial.printf("Turning COOL ON\n");
            currentState.setVal(2);
          }
          else if(currentTemp.getVal<float>()<=coolingThreshold.getVal<float>() && currentState.getVal()==2){
            Serial.printf("Turning COOL OFF\n");
            currentState.setVal(0);
          }
          break;
      }
  }

  // This "helper" function makes it easy to display temperatures on the serial monitor in either F or C depending on TemperatureDisplayUnits
  
  String temp2String(float temp){
    String t = displayUnits.getVal()?String(round(temp*1.8+32.0)):String(temp);
    t+=displayUnits.getVal()?" F":" C";
    return(t);    
  }  

};

////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.begin(Category::Thermostats,"HomeSpan Thermostat");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();

    new Reference_Thermostat();    
}

////////////////////////////////////////////////////////////////////////

void loop() {
  homeSpan.poll();
}

////////////////////////////////////////////////////////////////////////
