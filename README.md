# ESP32BLE
C++ BLE (Bluetooth Low Energy) library for the Espressif ESP32 microcontroller, using the VHCI API.

Primary motivation is the enormous code bloat from Bluedroid.

Program size of the `ESP32BLE.ino` working prototype:
```
Compiling 'ESP32BLE' for 'SparkFun ESP32 Thing'
esptool.py v2.3.1
Program size: 271,481 bytes (used 21% of a 1,310,720 byte maximum) (3.67 secs)
Minimum Memory Usage: 20660 bytes (7% of a 294912 byte maximum)
```

**VERY EARLY STAGE PROOF OF CONCEPT**

Ring Buffer class Copyright (c) 2015 D. Aaron Wisner under MIT license