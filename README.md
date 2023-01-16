# HomeSpanReferenceSketches

Comprehensive *Reference Sketches* showcasing some of the more complex HomeKit Services.  Built using the **[HomeSpan HomeKit Library](https://github.com/HomeSpan/HomeSpan)**, these sketches are designed to run on ESP32 devices under the Arduino IDE.  See [HomeSpan](https://github.com/HomeSpan/HomeSpan) for details.

This reposity includes Reference Sketches for the following HomeKit Services.  

* **[Thermostat](Thermostat/Thermostat.ino)**
  * Implements a complete Homekit Thermostat providing heating/cooling/auto/off modes
  * Includes a "simulated" temperature sensor allowing you to change the *current* temperature via the Serial Monitor to observe how the Thermostat responds in different modes
  * Includes stub code for monitoring and setting relative humidity

* **[Faucet](Faucet/Faucet.ino)**
  * Implements a multi-sprayer Shower System
  * Provides for central control of individual Shower Sprayers
  * Sprayers are implemented using linked HomeKit Valve Services
  * Allows you to operate, enable/disable and rename each Shower Sprayer from within the Home App
  * Note: The Home App can be a bit buggy with regards to this Service and sometimes shows a Sprayer is turned on when it is not

* **[Irrigation System](Irrigation/Irrigation.ino)**
  * Implements a multi-headed Sprinkler System
  * Provides full control of each Sprinkler Head, including the ability to set auto-off times from within the Home App
  * Heads are implemented using linked HomeKit Valve Services
  * Includes a configurable "Head Speed" setting that simulates the lag time it takes for water to actually start/stop flowing when a Head is opening/closing
  * Allows you to enable/disable and rename each Sprinkler Head from within the Home App
  * Includes the ability to run a "scheduled program" (which you start/stop via the Serial Monitor) causing each *enabled* Head to sequentially open/close  based on its specific duration time.  The schedule briefly pauses when switching from one Head to another to account for the Head Speed (in a real system this helps avoid sudden pressure drops)
  * Note: the Home App drop-down menu for selecting the duration time of each Sprinkler Head includes only a fixed number of choices determined by Apple (with the minimum time being 5 minutes).  The Eve for HomeKit App provides for more granular choices.  You can also set the duration time directly in the sketch using any number of seconds, from 1 to 3600, even if those times do not match an "allowed" choice shown in the Home App.  For illustration purposes the sketch is configured to initialize the Head Duration time for each value to be 20 seconds
  
  
### End Notes

1. These sketches are designed to demonstrate how various HomeKit Services work in practice with the Home App.  They do not include code that interfaces with actual hardware, such as a furnace, water valve, etc.  Instead, the code outputs messages to the Serial Monitoring reporting when a "simulated" activity occurs (such as a valve being turned on).  To interface with real-world applicances you will need to add your own code in the appropriate sections of each sketch.

1. Apple frequently changes the Home App interface as it updates Home Kit.  This sometimes causes specific functions to change the way they operate, how they are displayed, and even whether or not they continue to function at all.  Apple has not updated the HAP-R2 documentation since 2019 and some Characteristics listed in this documentation no longer function as indicated.  **These sketches have been tested under iOS 16.2.**

