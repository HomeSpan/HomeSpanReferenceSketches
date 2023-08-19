# HomeSpanReferenceSketches

Comprehensive *Reference Sketches* showcasing some of the more complex HomeKit Services.  Built using the **[HomeSpan HomeKit Library](https://github.com/HomeSpan/HomeSpan)**, these sketches are designed to run on ESP32 devices under the Arduino IDE.  See [HomeSpan](https://github.com/HomeSpan/HomeSpan) for details.

The following References Sketches have been tested under **iOS 16.5** (see note 2 below):  

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

* **[Battery Check](BatteryCheck/BatteryCheck.ino)**
  * Implements a simple on/off LED with a Battery Service to check battery level, charging status, and low-battery warning
  * Includes a stand-alone class to measure LiPo battery voltage and charging status when using an [Adafruit Huzzah32 Feather Board](https://www.adafruit.com/product/3405)

* **[Humidifier/Dehumidifier](Humidifier-Dehumidifier/Humidifier-Dehumidifier.ino)**
  * Implements a complete Homekit Humidifier/Dehumidifier providing humidify/dehumidify/auto/off modes
  * Includes a "simulated" humidity sensor allowing you to change the *current* humidity via the Serial Monitor to observe how the Humidifier/Dehumidifier responds in different modes
  * Includes stub code for optional water level, fan rotation speed, and swing modes
  * Shows how to restrict allowed modes to Humidify-only or Dehumidify-only
  
### End Notes

1. These sketches are designed to demonstrate how various HomeKit Services work in practice with the Home App.  They do not include code that interfaces with actual hardware, such as a furnace, water valve, etc.  Instead, the code outputs messages to the Serial Monitoring reporting when a "simulated" activity occurs (such as a valve being turned on).  To interface with real-world applicances you will need to add your own code in the appropriate sections of each sketch.

1. Apple frequently changes the Home App interface and underlying HomeKit architecture as it releases new versions of its operating system.  This sometimes causes specific functions to change the way they operate, how they are displayed, and even whether or not they continue to function at all.  As a result, aspects of the sketches above may, or may not, work as expected in future releases of Apple iOS.  Apple presumably informs manufactures of HomeKit products with commercial licenses of these changes, but Apple has not updated its non-commercial HAP documentation (which is used by HomeSpan) since version 2 was published in 2019.  It is already apparent that some Characteristics listed in the HAP-R2 documentation no longer function as indicated.  As a result, the only way to ensure sketches continue to work is by testing and experimentation whenever Apple releases new version of iOS.

