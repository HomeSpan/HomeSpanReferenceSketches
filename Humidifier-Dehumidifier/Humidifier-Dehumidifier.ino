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

////////////////////////////////////////////////////////////////////
//                                                                //
//   HomeSpan Reference Sketch: Humidifier/DeHumidifier Service   //
//                                                                //
////////////////////////////////////////////////////////////////////

#include "HomeSpan.h"

////////////////////////////////////////////////////////////////////////

// Here we create a dummmy humidity sensor that can be used as a real sensor in the Humidifier Service below.
// Rather than read a real humidity sensor, this structure allows you to change the current humidifier via the Serial Monitor

struct DummyHumiditySensor {
  static float relativeHumidity;
  
  DummyHumiditySensor(float rH) {
    relativeHumidity=rH;
    new SpanUserCommand('f',"<humidity> - set the relative humidity in percent [0-100]", [](const char *buf){relativeHumidity=atof(buf+1);});
  }

  float read() {return(relativeHumidity);}
};

float DummyHumiditySensor::relativeHumidity;

////////////////////////////////////////////////////////////////////////

struct Reference_HumidifierDehumidifier : Service::HumidifierDehumidifier {

  // Create characteristics, set initial values, and set storage in NVS to true

  Characteristic::Active active{0,true};
  Characteristic::CurrentRelativeHumidity humidity{70,true};
  Characteristic::CurrentHumidifierDehumidifierState currentState{0,true};
  Characteristic::TargetHumidifierDehumidifierState targetState{0,true};
  Characteristic::RelativeHumidityHumidifierThreshold humidThreshold{40,true};
  Characteristic::RelativeHumidityDehumidifierThreshold dehumidThreshold{80,true};
  Characteristic::SwingMode swing{0,true};
  Characteristic::WaterLevel water{50,true};

  DummyHumiditySensor humiditySensor{70};                                             // instantiate a dummy humidity sensor with initial humidity=70%
 
  Reference_HumidifierDehumidifier() : Service::HumidifierDehumidifier() {
    Serial.printf("\n*** Creating HomeSpan Humidifier/DeHumidifer ***\n");
    
//    targetState.setValidValues(1,1);      // uncomment this to restrict allowed mode to Humidify only
//    targetState.setValidValues(1,2);      // uncomment this to restrict allowed mode to Dehumidify only

  }

  boolean update() override {

    if(active.updated()){
      Serial.printf("Humidifier/DeHumidifier Power is %s\n",active.getNewVal()?"ON":"OFF");      
    }

    if(swing.updated()){
      Serial.printf("Humidifier/DeHumidifier Swing Mode is %s\n",swing.getNewVal()?"ON":"OFF");      
    }    

    if(targetState.updated()){
      switch(targetState.getNewVal()){
        case 0:
          Serial.printf("Mode set to AUTO\n");
          break;
        case 1:
          Serial.printf("Mode set to HUMIDIFY\n");
          break;
        case 2:
          Serial.printf("Mode set to DEHUMIDIFY\n");
          break;
      }
    }

    // NOTE:  HomeKit always updates both thresholds even if you only change one
    
    if(humidThreshold.updated() && humidThreshold.getNewVal<float>()!=humidThreshold.getVal<float>())
      Serial.printf("Humidifier Threshold changed to %g\n",humidThreshold.getNewVal<float>());
      
    if(dehumidThreshold.updated() && dehumidThreshold.getNewVal<float>()!=dehumidThreshold.getVal<float>())
      Serial.printf("Dehumidifier Threshold changed to %g\n",dehumidThreshold.getNewVal<float>());
    
    return(true);
  }

  // Here's where all the main logic exists to turn on/off humidifying/dehumidifying by comparing the current humidity to the devices's current settings

  void loop() override {

    float humid=humiditySensor.read();       // read humidity sensor (which in this example is just a dummy sensor)
    
    if(humid<0)                   // limit value to stay between 0 and 100
      humid=0;
    if(humid>100)
      humid=100;

    if(humidity.timeVal()>5000 && fabs(humidity.getVal<float>()-humid)>0.25){      // if it's been more than 5 seconds since last update, and humidity has changed
      humidity.setVal(humid);                                                       
      Serial.printf("Current humidity is now %g\n",humidity.getVal<float>());
    } 

    if(active.getVal()==0){                                               // power is OFF
      if(currentState.getVal()!=0){                                       // if current state is NOT Inactive
        Serial.printf("Humidifier/DeHumidifier State: INACTIVE\n");       // set to Inactive
        currentState.setVal(0);
      }
      return;                                                             // return since there is nothing more to check when device if OFF
    }

    if(humidity.getVal<float>()<humidThreshold.getVal<float>() && targetState.getVal()!=2){    // humidity is too low and mode allows for humidifying
      if(currentState.getVal()!=2){                                                          // if current state if NOT humidifying
        Serial.printf("Humidifier/DeHumidifier State: HUMIDIFYING\n");                       // set to Humidifying
        currentState.setVal(2);
      }
     return;
    }

    if(humidity.getVal<float>()>dehumidThreshold.getVal<float>() && targetState.getVal()!=1){  // humidity is too high and mode allows for dehumidifying
      if(currentState.getVal()!=3){                                                          // if current state if NOT dehumidifying
        Serial.printf("Humidifier/DeHumidifier State: DE-HUMIDIFYING\n");                    // set to Dehumidifying
        currentState.setVal(3);
      }
     return;
    }

    if(currentState.getVal()!=1){                                         // state should be Idle, but it is NOT
       currentState.setVal(1);
       Serial.printf("Humidifier/DeHumidifier State: IDLE\n");            // set to Idle        
    }
  }

};

////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.begin(Category::Humidifiers,"HomeSpan Humidifier");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();

    new Reference_HumidifierDehumidifier();    
}

////////////////////////////////////////////////////////////////////////

void loop() {
  homeSpan.poll();
}

////////////////////////////////////////////////////////////////////////
