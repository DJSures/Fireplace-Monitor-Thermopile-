# Fireplace-Monitor-Thermopile-
This uses an Hetec Wifi Kit 32, speaker on the DAC, and ADC connected to fireplace thermopile. It will monitor a fireplace pilot light and notify your phone with push notifications and an audible alarm.

Edit the Arduino project INO file to see the #defines. You can configure your WiFi settings and PushOver token/user key information. 

   1) Choose the wifi mode by uncomment AP_MODE or not
   2) Enter the SSID/PWD for either AP or Client mode
   3) Connect the Pilot voltage to A7 (pin 35)
   4) Connect the heat voltage (optional) to A6 (pin 34)
   5) Get USER Key and API Token from https://pushover.net/api
   6) Edit the #defines below
