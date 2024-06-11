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

#define  HEAD_DURATION    20       // default duration, in seconds, for each Head to stay open (can be configured for each Head in Home App)
#define  HEAD_SPEED       5000     // the time, in milliseconds, it takes for valve to open/close and water to fully flow or stop flowing

////////////////////////////////////////////////////////////////////////

struct Head : Service::Valve {

  SpanCharacteristic *active=new Characteristic::Active(0);
  SpanCharacteristic *inUse=new Characteristic::InUse(0);
  SpanCharacteristic *enabled = new Characteristic::IsConfigured(1,true);
  SpanCharacteristic *setDuration = new Characteristic::SetDuration(HEAD_DURATION);
  SpanCharacteristic *remainingDuration = new Characteristic::RemainingDuration(0);
  SpanCharacteristic *name;

  Head(const char *headName) : Service::Valve() {
    new Characteristic::ValveType(1);                           // Set Valve Type = Irrigation
    name=new Characteristic::ConfiguredName(headName,true);     
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

    // Below we simulate valves that take some fixed time to open/close so that changes in InUse lags changes in Active.
    // The Home App will accurately reflect this intermediate state by displaying "Waiting..." when a value is initially opened.
    
    if(active->timeVal()>HEAD_SPEED && active->getVal()!=inUse->getVal()){
      inUse->setVal(active->getVal());
      Serial.printf("%s value is %s\n",name->getString(),inUse->getVal()?"OPEN":"CLOSED");
    }
  }

};

////////////////////////////////////////////////////////////////////////

struct Sprinkler : Service::IrrigationSystem {

  // In this configuration we will LINK one or more Heads (i.e. Valve Services) to the Irrigation Service, INSTEAD of
  // having the Irrigation Service be a standalone Service with unlinked Valves.  This means that the Home App will
  // not display a separate "master" control for the Irrigation Service, and will instead automatically determine whether
  // the system is Active according to whether one more values are Active.

  SpanCharacteristic *programMode=new Characteristic::ProgramMode(0);   // HomeKit requires this Characteristic, but it is only for display purposes in the Home App
  SpanCharacteristic *active=new Characteristic::Active(0);             // though in this configuration the Home App will not display a "master" control, the Active Characteristic is still required
  SpanCharacteristic *inUse=new Characteristic::InUse(0);               // though in this configuration the Home App will not display a "master" control, the InUse Characteristic is still required
  
  vector<Head *> heads;                                                 // OPTIONAL: vector to store list of linked Heads that will be used for running a scheduled program
  vector<Head *>::iterator currentHead;                                 // OPTIONAL: pointer to the current head in a scheduled program

  Sprinkler(uint8_t numHeads) : Service::IrrigationSystem() {
    
    for(int i=1;i<=numHeads;i++){              // create Heads (Valves) and link each to the Sprinkler object
      char name[16];
      sprintf(name,"Head-%d",i);               // unique name for each Head that can be changed in the Home App
      addLink(new Head(name));
    }
    
    for(auto s : getLinks())                  // OPTIONAL: add each linked Head into storage vector for easy access below
      heads.push_back((Head *)s);
      
    new SpanUserCommand('p', "- starts/stops scheduled program",startProgram,this);     // OPTIONAL: allows user to start a schedule program to sequentially turn on each enabled Head
  }

  static void startProgram(const char *buf, void *arg){     // OPTIONAL: start scheduled program
    
    Sprinkler *sprinkler=(Sprinkler *)arg;                      // recast the second arguments into a Sprinkler
        
    for(auto s : sprinkler->getLinks()) {                       // loop over all linked Services
      Head *head = (Head *)s;                                   // recast linked Service to a Head
      if(head->enabled->getVal() && head->active->getVal())     // if Head is both enabled and active
        head->active->setVal(0);                                // turn off Heads
    }

    sprinkler->currentHead=sprinkler->heads.begin();                       // reset current head in scheduled program
    sprinkler->active->setVal(0);                                          // set sprinkler Active to false
    sprinkler->programMode->setVal(!sprinkler->programMode->getVal());     // toggle Program Mode

    Serial.printf("Scheduled program: %s\n",sprinkler->programMode->getVal()?"STARTED":"STOPPED");
  }

  void loop(){                                              // OPTIONAL: only needed to support the running of scheduled programs
     
    if(!programMode->getVal())      // program mode not started
      return;

    if(currentHead==heads.end()){
      Serial.printf("Scheduled program: COMPLETED\n");
      programMode->setVal(0);
      active->setVal(0);
      return;
    }

    if(!(*currentHead)->enabled->getVal()){      // current Head is not enabled
      currentHead++;                             // advance to next Head
      return;
    }

    if((*currentHead)->active->getVal()){        // current Head is Active
      if(!active->getVal()){                     // if sprinkler Active is still false (because user manually turned on Head)...
        active->setVal(1);                       // ...set sprinkler Active to true
        Serial.printf("Scheduled program: %s is ALREADY OPEN\n",(*currentHead)->name->getString());
      }
      return;
      
    } else if((*currentHead)->inUse->getVal()){  // current Head is not Active but still InUse
      return;
    }
    
    if(!active->getVal()){                       // current Head is not Active nor InUse and sprinkler Active is false...
      active->setVal(1);                         // ...set sprinkler Active to true and turn on Head
      (*currentHead)->active->setVal(1);
      (*currentHead)->remainingDuration->setVal((*currentHead)->setDuration->getVal());
      Serial.printf("Scheduled program: %s is OPENING\n",(*currentHead)->name->getString());
      
    } else if(!(*currentHead)->inUse->getVal()){  // wait for water to stop flowing in current Head before moving to next Head
      active->setVal(0,false);
      currentHead++;      
    }
    
  } // loop()

};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  
  homeSpan.begin(Category::Sprinklers,"HomeSpan Sprinklers");

  new SpanAccessory();                                  
    new Service::AccessoryInformation();  
      new Characteristic::Identify();                           
                   
    new Sprinkler(4);       // create Sprinkler with 4 Heads
}

////////////////////////////////////////////////////////////////////////

void loop(){ 
  homeSpan.poll();  
}

////////////////////////////////////////////////////////////////////////
