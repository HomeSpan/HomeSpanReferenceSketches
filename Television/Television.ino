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
  
  TvInput(const char *name) : Service::InputSource() {
    sourceName = new Characteristic::ConfiguredName(name,true);
    sourceID = new Characteristic::Identifier(++numSources);
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

int TvInput::numSources=0;

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
  Characteristic::DisplayOrder displayOrder{"",true};      // this TLV8 characteristic is used to set the order in which the Input Sources are displayed in the Home App                         
  
  SpanCharacteristic *tvName;                              // name of TV (will be instantiated in constructor below)

  HomeSpanTV(const char *name) : Service::Television() {
    tvName = new Characteristic::ConfiguredName(name,true);
    Serial.printf("Creating Television Service '%s'\n",tvName->getString()); 

    addLink(new TvSpeaker());                              // link speaker first so the code further above knows to ignore when searching for name of source 
    addLink(new TvInput("HDMI-1"));                        // add Input Sources; the TvInput constructor above automatically assigns an ID=1 to first input source, ID=2 to second, etc.
    addLink(new TvInput("HDMI-2"));
    addLink(new TvInput("HDMI-3"));
    addLink(new TvInput("HDMI-4"));
    addLink(new TvInput("HDMI-5"));

    // Unless we specifiy an order for the Input Sources, the Home App displayes them in a random order
    // Below we create a TLV8 record set so that Input Source with ID=1 is displayed first, then ID=2, etc, to match the order above

    TLV8 orderTLV;                                         // create an empty TLV8 object to store the order in which the Input Sources are displayed in the Home App

    for(int i=1;i<getLinks().size();i++){                  // loop over all input sources - note we don't start i=0 since very first linked service is a speaker
      orderTLV.add(1,i);                                   // add TLV8 record with TAG=1 and VALUE=i, which will match the ID of each input source in the order created above
      orderTLV.add(0);                                     // add empty TLV8 record with TAG=0 to be used as a record spacer
    }

    displayOrder.setTLV(orderTLV);                       // set the "value" of displayOrder to be the fully-filled orderTLV object
    orderTLV.print();
  }

  boolean update() override {

    if(power.updated()){
      Serial.printf("Set TV Power to: %s\n",power.getNewVal()?"ON":"OFF");
    }

    if(inputSource.updated()){
      for(int i=1;i<getLinks().size();i++){                // loop over all input sources - note we don't start i=0 since very first linked service is a speaker (see above)
        TvInput *tvInput = (TvInput *)getLinks()[i];
        if(inputSource.getNewVal()==tvInput->sourceID->getVal())
          Serial.printf("Set to Input Source %d: %s\n",tvInput->sourceID->getVal(),tvInput->sourceName->getString());
      }
    }

    if(settingsKey.updated()){
      Serial.printf("Received request to \"View TV Settings\"\n");
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
