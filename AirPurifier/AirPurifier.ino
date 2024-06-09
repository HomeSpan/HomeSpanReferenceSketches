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

 
/////////////////////////////////////////////////////////
//                                                     //
//   HomeSpan Reference Sketch: Air Purifier Service   //
//                                                     //
/////////////////////////////////////////////////////////

// As of iOS 17.2.1, the Home App provides the following Air Purifier functionality:
// 
//  * a Master Switch allows you to set the Active Characteristic to either ACTIVE or INACTIVE
//  * a duplicate of this same functionality is available via a toggle button labeled "Mode" on the 
//    Settings Screen that also allows you to set the Active Characteristic to either ACTIVE or INACTIVE
//  * two selector buttons on the Settings Screen allow you to set the TargetAirPurifierState Characteristic
//    to either AUTO or MANUAL
//
//    NOTE: When changing the state of the Accessory from INACTIVE to ACTIVE via either the Master Switch or
//    the duplicate "Mode" toggle on the Settings Screen, the Home App automatically sets TargetAirPurifierState
//    to AUTO.  If you want to operate in MANUAL mode, you must select that option *after* first setting
//    the Accessory to ACTIVE.  In other words, the Home App always "starts" the Purifier in AUTO mode
//
 
#include "HomeSpan.h"

////////////////////////////////////////////////////////////////////////

struct AirFilter : Service::FilterMaintenance {

  Characteristic::FilterChangeIndication filterChange;
  Characteristic::FilterLifeLevel filterLife;
  Characteristic::ResetFilterIndication filterReset;  
  Characteristic::ConfiguredName filterName;

  AirFilter(const char *name) : Service::FilterMaintenance() {
    filterName.setString(name);
  }

  boolean update() override {
    if(filterReset.updated()){
      filterLife.setVal(100);                               // reset filter life to 100%
      filterChange.setVal(filterChange.NO_CHANGE_NEEDED);   // reset filter change indicator
    }

    return(true);
  }
  
};

////////////////////////////////////////////////////////////////////////

struct AirSensor : Service::AirQualitySensor {

  Characteristic::AirQuality airQuality{Characteristic::AirQuality::GOOD};
  Characteristic::OzoneDensity ozoneDensity{100};
  Characteristic::NitrogenDioxideDensity no2Density{200};
  Characteristic::SulphurDioxideDensity so2Density{300};
  Characteristic::PM25Density smallPartDensity{400};
  Characteristic::PM10Density largePartDensity{500};
  Characteristic::VOCDensity vocDensity{600};  
};  

////////////////////////////////////////////////////////////////////////

struct Purifier : Service::AirPurifier {

  Characteristic::Active active;
  Characteristic::CurrentAirPurifierState currentState;
  Characteristic::TargetAirPurifierState targetState;

  AirSensor airSensor;
  AirFilter preFilter{"Pre-Filter"};
  AirFilter hepaFilter{"HEPA Filter"};

  Purifier() : Service::AirPurifier() {

    addLink(&preFilter);          // AirFilters need to be linked to the AirPurifier to show up in HomeKit, but will work in Eve even if not linked
    addLink(&hepaFilter);
  }

  boolean update() override {

    if(active.updated()){
      if(active.getNewVal()==active.ACTIVE)
        LOG0("Purifier ACTIVE (AUTO)\n");
      else
        LOG0("Purifier INACTIVE\n");
    }

    if(targetState.updated() && active.getVal()==active.ACTIVE){
      if(targetState.getVal()==targetState.AUTO)
        LOG0("Purifier ACTIVE (AUTO)\n");
      else
        LOG0("Purifier ACTIVE (MANUAL)\n");
    }
    
    return(true);
  }

  void loop() override {

    // if the master switch is set to INACTIVE, make sure the Accessory is also INACTIVE regardless of whether in MANUAL or AUTO mode

    if(active.getVal()==active.INACTIVE && currentState.getVal()!=currentState.INACTIVE){
      LOG0("Purifier is turning OFF.\n");
      currentState.setVal(currentState.INACTIVE);
    }

    // if in MANUAL mode and the master switch is set to ACTIVE, make sure the Accessory is PURIFYING
    
    else if(targetState.getVal()==targetState.MANUAL && active.getVal()==active.ACTIVE && currentState.getVal()!=currentState.PURIFYING){
      LOG0("Purifier is PURIFYING.\n");
      currentState.setVal(currentState.PURIFYING);
    }
    
    // if in AUTO mode and the master switch is set to ACTIVE, make sure the Accessory is either PURIFYING or IDLE depending on Air Quality Sensor

    else if(targetState.getVal()==targetState.AUTO && active.getVal()==active.ACTIVE && currentState.getVal()==currentState.INACTIVE){
      LOG0("Purifier is IDLE.\n");
      currentState.setVal(currentState.IDLE);      
    }
    
  }
};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  homeSpan.setLogLevel(2);
  
  homeSpan.begin(Category::AirPurifiers,"HomeSpan Purifier");

  new SpanAccessory();                                  
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      
  new SpanAccessory();                                  
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("HEPA Purifier");
                    
    new Purifier;
}

////////////////////////////////////////////////////////////////////////

void loop(){ 
  homeSpan.poll();  
}

////////////////////////////////////////////////////////////////////////
