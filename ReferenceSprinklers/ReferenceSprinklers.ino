/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2021 Gregg E. Berman
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

 
  ////////////////////////////////////////////////////////
  //                                                    //
  //    HomeSpan Reference SPRINKLERS Sketch:           //
  //                                                    //
  //    * IRRIGATION Service                            //
  //    * Multiple linked VALVE Services                //
  //                                                    //  
  ////////////////////////////////////////////////////////

 
#include "HomeSpan.h" 

struct Head : Service::Valve {

  SpanCharacteristic *active=new Characteristic::Active(0);
  SpanCharacteristic *inUse=new Characteristic::InUse(0);
  SpanCharacteristic *enabled = new Characteristic::IsConfigured(1);
  SpanCharacteristic *setDuration = new Characteristic::SetDuration(30);
  SpanCharacteristic *remainingDuration = new Characteristic::RemainingDuration(0);
  SpanCharacteristic *name;

  Head(const char *headName) : Service::Valve() {
    new Characteristic::ValveType(1);
    name=new Characteristic::ConfiguredName(headName);
    enabled->perms|=PW;
  }

  boolean update() override {
    
    if(enabled->updated()){
      if(enabled->getNewVal()){
        Serial.printf("Head '%s' enabled\n",name->getString());
      } else {
        Serial.printf("Head '%s' disabled\n",name->getString());          
        if(active->getVal()){
          active->setVal(0);
          inUse->setVal(0);
          remainingDuration->setVal(0);
          Serial.printf("Head '%s' valve is closing\n",name->getString());          
        }
      }
    }

    if(active->updated()){
      if(active->getNewVal()){
        Serial.printf("Head '%s' valve is opening\n",name->getString());
        inUse->setVal(1);
        remainingDuration->setVal(setDuration->getVal());
      } else {
        Serial.printf("Head '%s' valve is closing\n",name->getString());
        inUse->setVal(0);
        remainingDuration->setVal(0);
      }
    }
    
    return(true);
  }

  void loop() override {
    if(active->getVal() && (active->timeVal() > setDuration->getVal()*1000)){
      Serial.printf("Head '%s' is closing (%d-second timer is complete)\n",name->getString(),setDuration->getVal());
      active->setVal(0);
      inUse->setVal(0);
      remainingDuration->setVal(0);
    }
  }

};

struct Sprinkler : Service::IrrigationSystem {

  SpanCharacteristic *active=new Characteristic::Active(0);
  SpanCharacteristic *programMode=new Characteristic::ProgramMode(0);
  
};

//////////////////////////////////////

void setup() {

  Serial.begin(115200);
  
  homeSpan.begin(Category::Sprinklers,"HomeSpan Sprinklers");

  new SpanAccessory();                                  

    new Service::AccessoryInformation();
      new Characteristic::Name("Sprinkler System");                   
      new Characteristic::Manufacturer("HomeSpan");             
      new Characteristic::SerialNumber("HSL-123");              
      new Characteristic::Model("HSL Test");                    
      new Characteristic::FirmwareRevision(HOMESPAN_VERSION);   
      new Characteristic::Identify();                           
  
    new Service::HAPProtocolInformation();
      new Characteristic::Version("1.1.0");                     
     
    (new Sprinkler())
      ->addLink(new Head("Head 1"))
      ->addLink(new Head("Head 2"))
      ->addLink(new Head("Head 3"))
      ->addLink(new Head("Head 4"))
      ;

} // end of setup()

//////////////////////////////////////

void loop(){
  
  homeSpan.poll();
  
} // end of loop()

//////////////////////////////////////
