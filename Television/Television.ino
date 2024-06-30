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

struct sourceData_t {
  int ID;
  const char *name;
};

#define   NUM_SOURCES     5

sourceData_t sourceData[NUM_SOURCES]={
  30,"HDMI-1",
  40,"HDMI-2",
  10,"Component-1",
  25,"Component-2",
  15,"DVI"
};

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
  
  SpanCharacteristic *tvName;                                   // name of TV (will be instantiated in constructor below)
  Characteristic::DisplayOrder displayOrder{NULL_TLV,true};     // sets the order in which the Input Sources will be displayed in the Home App

  HomeSpanTV(const char *name) : Service::Television() {
    tvName = new Characteristic::ConfiguredName(name,true);
    
    Serial.printf("Creating Television Service '%s'\n",tvName->getString());

    TLV8 orderTLV;                                         // create a temporary TLV8 object to store the order in which the Input Sources are to be displayed in the Home App

    for(int i=0;i<NUM_SOURCES;i++){
      orderTLV.add(1,sourceData[i].ID);                    // add ID of Input Source to TLV8 record used for displayOrder
      orderTLV.add(0);
      
      addLink(new TvInput(sourceData[i].ID,sourceData[i].name));    // add link for this Input Source
    }

    displayOrder.setTLV(orderTLV);                         // update displayOrder with completed TLV8 records

    addLink(new TvSpeaker());                              // add link for TV Speaker
  }

  boolean update() override {

    if(power.updated()){
      Serial.printf("Set TV Power to: %s\n",power.getNewVal()?"ON":"OFF");
    }

    if(inputSource.updated()){                                  // request for new Input Source
      for(auto src : getLinks<TvInput *>("InputSource"))        // loop through all linked TvInput Sources and find one with matching ID
        if(inputSource.getNewVal()==src->sourceID->getVal())
          Serial.printf("Set to Input Source %d: %s\n",src->sourceID->getVal(),src->sourceName->getString());          
    }

    // for fun, use "View TV Settings" to trigger HomeSpan to re-order the Input Sources alphabetically
    
    if(settingsKey.updated()){
      Serial.printf("Received request to \"View TV Settings\"\n");
      
      // for fun, use "View TV Settings" to trigger HomeSpan to re-order the Input Sources alphabetically
      
      Serial.printf("Alphabetizing Input Sources...\n");
      auto inputs = getLinks<TvInput *>("InputSource");         // create copy of linked Input Source vector so it can be sorted alphabetically
                    
      std::sort(inputs.begin(),inputs.end(),[](TvInput *i, TvInput *j)->boolean{return(strcmp(i->sourceName->getString(),j->sourceName->getString())<0);});

      TLV8 orderTLV;                                            // create a temporary TLV8 object to store the order in which the Input Sources are to be displayed in the Home App

      for(auto src : inputs){
        orderTLV.add(1,src->sourceID->getVal());
        orderTLV.add(0);
      }

    displayOrder.setTLV(orderTLV);                              // update displayOrder Characteristic with TLV8 record
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
