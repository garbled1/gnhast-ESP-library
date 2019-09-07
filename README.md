# gnhast-ESP-library
A basic collector library for ESP8266

## Notes on this library

* Needs a gnhast daemon to connect to, see https://github.com/garbled1/gnhast
* Gnhast is a scale-less engine.  What this means is it doesn't care what 
  format you feed it data in, unless you care.
  
  For example, lets say you like Celcius instead of Farenheit.  Just put the
  data in as C.  Gnhast will store it as C, and when you ask it for the current
  data, it will give you C.  Now if when you store the data, you tell gnhast
  the device is scale of type C, by setting scale to TSCALE_C, gnhast will
  still store the data in Ceclius, but if you ask it for the data in F, it
  will convert it to F for you on the reply. Not every type of scale is
  implemented yet, but most are.
  
  So all that being said, if you like Celcius, just load your data in C. Don't
  sweat it.  It only truly matters if some insane device only reports in F.
  Even then, maybe just do the math in the collector first.
  
* See more protocol docs here: https://codedocs.xyz/garbled1/gnhast/
* And here: https://garbled1.github.io/gnhast/

## Requires the following libraries
* ArduinoJson (v6)
* ESPAsyncTCP
* ESPAsyncWebServer
* ESPAsyncWiFiManager
