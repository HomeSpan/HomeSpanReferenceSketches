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
//   HomeSpan Reference Sketch: Irrigation Service   //
//                                                   //
///////////////////////////////////////////////////////
 
#include "HomeSpan.h"

struct Sprinkler *sprinklerSystem;      // create global pointer to an instance of Sprinkler so we can reference it inside SpanUserCommand later on

////////////////////////////////////////////////////////////////////////

struct Sprinkler : Service::IrrigationSystem {

  SpanCharacteristic *active=new Characteristic::Active(0);             // HomeKit requires this Characteristic, but it has no effect in Home App
  SpanCharacteristic *programMode=new Characteristic::ProgramMode(0);   // HomeKit requires this Characteristic, but it is mostly for information purposeses only in the Home App

  // NOTE: According to HAP-R2, the In Use Characteristic is also required for the Irrigation System Service.
  // However, adding this Characteristic seems to break the Home App.  Seems that the HAP-R2 spec is out of date
  // and that the Home App determines whether the Irrigation System is In Use simply by noting that one or more
  // linked Valves are In Use.
  //
  // Recommendation: Do NOT instatiate Characteristic::InUse for the Irrigation Service

};

////////////////////////////////////////////////////////////////////////

struct Head : Service::Valve {

  SpanCharacteristic *active=new Characteristic::Active(0);
  SpanCharacteristic *inUse=new Characteristic::InUse(0);
  SpanCharacteristic *enabled = new Characteristic::IsConfigured(1,true);
  SpanCharacteristic *setDuration = new Characteristic::SetDuration(300);
  SpanCharacteristic *remainingDuration = new Characteristic::RemainingDuration(0);
  SpanCharacteristic *name;

  Head(const char *headName) : Service::Valve() {
    new Characteristic::ValveType(1);
    name=new Characteristic::ConfiguredName(headName,true);     // This Characteristic was introduced for TV Services, but works well here
    enabled->addPerms(PW);                                      // Adding "PW" to the IsConfigured Characteristic allows for enabling/disabling valves
  }

  boolean update() override {
    
    if(enabled->updated()){
      if(enabled->getNewVal()){
        Serial.printf("%s value ENABLED\n",name->getString());
      } else {
        Serial.printf("%s value DISABLED\n",name->getString());          
        if(active->getVal()){
          active->setVal(0);
          remainingDuration->setVal(0);
          Serial.printf("%s is CLOSING\n",name->getString());          
        }
      }
    }

    if(active->updated()){
      if(active->getNewVal()){
        Serial.printf("%s valve is OPENING\n",name->getString());
        remainingDuration->setVal(setDuration->getVal());
      } else {
        Serial.printf("%s valve is CLOSING\n",name->getString());
        remainingDuration->setVal(0);
      }
    }
    
    return(true);
  }

  void loop() override {
    if(active->getVal()){
      int remainingTime=setDuration->getVal()-active->timeVal()/1000;
         
      if(remainingTime<=0){
        Serial.printf("%s valve is CLOSING (%d-second timer is complete)\n",name->getString(),setDuration->getVal());
        active->setVal(0);
        remainingDuration->setVal(0);
      } else

      if(remainingTime<remainingDuration->getVal()){
        remainingDuration->setVal(remainingTime,false);
      }
    }

    // Below we simulate valves that take 5 seconds to open/close so that In Use follows Active by 5 seconds
    // The Home App will accurately reflect this intermediate state and show "Waiting..." when a value of initially
    // activated
    
    if(active->timeVal()>5000 && active->getVal()!=inUse->getVal()){
      inUse->setVal(active->getVal());
      Serial.printf("%s value is %s\n",name->getString(),inUse->getVal()?"OPEN":"CLOSED");
    }
  }

};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  
  homeSpan.begin(Category::Sprinklers,"HomeSpan Sprinklers");

  new SpanAccessory();                                  
    new Service::AccessoryInformation();  
      new Characteristic::Identify();                           
                   
    sprinklerSystem = new Sprinkler();
    sprinklerSystem->addLink(new Head("Head 1"))
                  ->addLink(new Head("Head 2"))
                  ->addLink(new Head("Head 3"))
                  ->addLink(new Head("Head 4"))
                  ;

  // This allows user to toggle programMode on/off from Serial Monitor, though this sketch does not contain any actual scheduled programs:
  
  new SpanUserCommand('p', "- starts/stops scheduled program",[](const char *buf){sprinklerSystem->programMode->setVal(!sprinklerSystem->programMode->getVal());});  

} // end of setup()

////////////////////////////////////////////////////////////////////////

void loop(){
  
  homeSpan.poll();
  
} // end of loop()

//////////////////////////////////////
