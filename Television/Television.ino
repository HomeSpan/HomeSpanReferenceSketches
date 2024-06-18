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

struct HomeSpanTV : Service::Television {

  Characteristic::Active power{0,true};                    // TV power
  Characteristic::ActiveIdentifier inputSource{1,true};    // current TV Input Source
  Characteristic::RemoteKey remoteKey;                     // used to receive button presses from the Remote Control widget
  Characteristic::PowerModeSelection settingsKey;          // adds "View TV Settings" option to Selection Screen  
  SpanCharacteristic *tvName;                              // name of TV

  HomeSpanTV(char *name) : Service::Television() {
    tvName = new Characteristic::ConfiguredName(name,true);
    Serial.printf("Creating Television Service '%s'\n",tvName->getString());  
  }

  boolean update() override {

    if(power.updated()){
      Serial.printf("Set TV Power to: %s\n",power.getNewVal()?"ON":"OFF");
    }

    if(inputSource.updated()){
      Serial.printf("Set Input Source to #%d \n",inputSource.getNewVal());        
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

///////////////////////////////

void setup() {
  
  Serial.begin(115200);
 
  homeSpan.begin(Category::Television,"HomeSpan Television");

  new SpanAccessory();   
    new Service::AccessoryInformation(); 
      new Characteristic::Identify();
      
    SpanService *speaker = new Service::TelevisionSpeaker();
      new Characteristic::VolumeSelector();
      new Characteristic::VolumeControlType(3);

    (new HomeSpanTV("Test TV"))                                  // define a Television Service with link in Input Sources and Speaker
      ->addLink(new TvInput("HDMI-1"))
      ->addLink(new TvInput("HDMI-2"))
      ->addLink(new TvInput("HDMI-3"))
      ->addLink(speaker);
      
}

///////////////////////////////

void loop() {
  homeSpan.poll();
}
