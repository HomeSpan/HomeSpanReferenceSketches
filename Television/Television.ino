/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2024 Gregg E. Berman
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
//   HomeSpan Reference Sketch: Television Service   //
//                                                   //
///////////////////////////////////////////////////////

#include "HomeSpan.h"

////////////////////////////////////////////////////////////////////////
///// TvInput Service - used by Television Service further below ///////
////////////////////////////////////////////////////////////////////////

struct TvInput : Service::InputSource {

  SpanCharacteristic *sourceName;                                    // name of input source
  SpanCharacteristic *sourceID;                                      // ID of input source
  Characteristic::IsConfigured configured{1,true};                   // indicates whether input source is configured
  Characteristic::CurrentVisibilityState currentVis{0,true};         // current input source visibility (0=VISIBLE)
  Characteristic::TargetVisibilityState targetVis{0,true};           // target input source visibility

  static int numSources;                                             // number of input sources
  
  TvInput(int id, const char *name) : Service::InputSource() {
    sourceName = new Characteristic::ConfiguredName(name,true);
    sourceID = new Characteristic::Identifier(id);
    Serial.printf("Creating Input Source %d: %s\n",sourceID->getVal(),sourceName->getString());
  }

  boolean update() override {

    if(targetVis.updated()){
      currentVis.setVal(targetVis.getNewVal());
      Serial.printf("Input Source %s is now %s\n",sourceName->getString(),currentVis.getVal()?"HIDDEN":"VISIBLE");
    }
    
    return(true);
  }
  
};

////////////////////////////////////////////////////////////////////////
///// TvSpeaker Service - used by Television Service further below /////
////////////////////////////////////////////////////////////////////////

struct TvSpeaker : Service::TelevisionSpeaker {

  Characteristic::VolumeSelector volumeChange;

  TvSpeaker() : Service::TelevisionSpeaker() {
    new Characteristic::VolumeControlType(3);
    Serial.printf("Adding Volume Control\n");
  }

  boolean update() override {
    if(volumeChange.updated())
      Serial.printf("Volume %s\n",volumeChange.getNewVal()?"DECREASE":"INCREASE");

    return(true);
  }

};

////////////////////////////////////////////////////////////////////////
/////               HomeSpanTV Television Service                ///////
////////////////////////////////////////////////////////////////////////

struct HomeSpanTV : Service::Television {

  Characteristic::Active power{0,true};                    // TV power
  Characteristic::ActiveIdentifier inputSource{1,true};    // current TV Input Source
  Characteristic::RemoteKey remoteKey;                     // used to receive button presses from the Remote Control widget
  Characteristic::PowerModeSelection settingsKey;          // adds "View TV Settings" option to Selection Screen
  
  SpanCharacteristic *tvName;                              // name of TV (will be instantiated in constructor below)

  Characteristic::DisplayOrder displayOrder{"",true};      // this TLV8 characteristic is used to set the order in which the Input Sources are displayed in the Home App                         
  vector<TvInput *> inputs;                                // vector of pointers to Input Sources used for creating displayOrder

  HomeSpanTV(const char *name) : Service::Television() {
    tvName = new Characteristic::ConfiguredName(name,true);
    Serial.printf("Creating Television Service '%s'\n",tvName->getString()); 

    addLink(new TvInput(30,"HDMI-1"));                     // add Input Sources (unique ID and default name)
    addLink(new TvInput(40,"HDMI-2"));
    addLink(new TvInput(10,"Component-1"));
    addLink(new TvInput(20,"Component-2"));
    addLink(new TvInput(50,"DVI"));

    for(auto source : getLinks<TvInput *>())               // loop over all sources in getLines and add them to inputs vector             
      inputs.push_back(source);
      
    addLink(new TvSpeaker());                              // add TV Speaker (do this AFTER creating inputs vector from getLinks above)

    updateDisplayOrder(inputs);
  }

  boolean update() override {

    if(power.updated()){
      Serial.printf("Set TV Power to: %s\n",power.getNewVal()?"ON":"OFF");
    }

    if(inputSource.updated()){                                  // request for new Input Source
      for(auto src : inputs)                                    // loop through all sources in inputs vector and find one with matching ID
        if(inputSource.getNewVal()==src->sourceID->getVal())
          Serial.printf("Set to Input Source %d: %s\n",inputSource.getNewVal(),src->sourceName->getString());          
    }

    // for fun, use "View TV Settings" to trigger HomeSpan to re-order the Input Sources alphabetically
    
    if(settingsKey.updated()){
      Serial.printf("Received request to \"View TV Settings\" --- alphabetizing Input Sources\n");      
      std::sort(inputs.begin(),inputs.end(),[](TvInput *i, TvInput *j)->boolean{return(strcmp(i->sourceName->getString(),j->sourceName->getString())<0);});
      updateDisplayOrder(inputs);
    }
    
    if(remoteKey.updated()){
      Serial.printf("Remote Control key pressed: ");
      switch(remoteKey.getNewVal()){
        case 4:
          Serial.printf("UP ARROW\n");
          break;
        case 5:
          Serial.printf("DOWN ARROW\n");
          break;
        case 6:
          Serial.printf("LEFT ARROW\n");
          break;
        case 7:
          Serial.printf("RIGHT ARROW\n");
          break;
        case 8:
          Serial.printf("SELECT\n");
          break;
        case 9:
          Serial.printf("BACK\n");
          break;
        case 11:
          Serial.printf("PLAY/PAUSE\n");
          break;
        case 15:
          Serial.printf("INFO\n");
          break;
        default:
          Serial.print("UNKNOWN KEY\n");
      }
    }

    return(true);
  }

  void updateDisplayOrder(vector<TvInput *> & sources){    // this optional function updates the displayOrder Characteristic to reflect the order of the TvInputs in the specified "sources" vector

    TLV8 orderTLV;                                         // create a temporary TLV8 object to store the order in which the Input Sources are to be displayed in the Home App

    for(auto src : sources){                               // loop over all input sources
      orderTLV.add(1,src->sourceID->getVal());             // add TLV8 record with TAG=1 and VALUE=ID of TvInput source
      orderTLV.add(0);                                     // add empty TLV8 record with TAG=0 to be used as a record separator
    }

    displayOrder.setTLV(orderTLV);                         // set the "value" of displayOrder to be the fully-filled orderTLV object
  }
};

////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.enableWebLog(0);
  homeSpan.begin(Category::Television,"HomeSpan Television");

  new SpanAccessory();   
    new Service::AccessoryInformation(); 
      new Characteristic::Identify();
    new HomeSpanTV("Test TV");              // instantiate a Television with name="Test TV"
}

///////////////////////////////

void loop() {
  homeSpan.poll();
}
