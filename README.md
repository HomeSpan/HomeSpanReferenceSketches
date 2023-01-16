# HomeSpanReferenceSketches

Comprehensive *Reference Sketches* showcasing some of the more complex HomeKit Services.  Built using the [HomeSpan](https://github.com/HomeSpan/HomeSpan) HomeKit Library, these sketches are designed to run on ESP32 devices under the Arduino IDE.  See [HomeSpan](https://github.com/HomeSpan/HomeSpan) for details.

This reposity includes Reference Sketches for the following HomeKit Services.  

* **Thermostat**
  * Implements a complete Homekit Thermostat providing heating/cooling/auto/off modes
  * Includes a "simulated" temperature sensor allowing you to change the *current* temperature via the Serial Monitor to observe how the Thermostat responds in different modes
  * Includes stub code for monitoring and setting relative humidity
  
  
> Note that these sketches are designed to demonstrate how various HomeKit Services work in practice with the Home App.  They do not include code that interfaces with actual hardware, such as a furnace, water valve, etc.  Instead, the code outputs messages to the Serial Monitoring reporting when a "simulated" activity occurs (such as a valve being turned on).  To interface with real-world applicances you will need to add your own code in the appropriate sections of each sketch.
