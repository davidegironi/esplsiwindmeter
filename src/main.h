/*
ESP LSI Wind Meter v01

copyright (c) Davide Gironi, 2020

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#ifndef main_h
#define main_h

#include <Wire.h>

//ADS1X15 library
#include <Adafruit_ADS1015.h>

//set the serial output debug
#define SERIAL_ENABLED 1
//set the serial output debug
#define SERIALDEBUGSENSOR_ENABLED 1

//connection status led
#define WIFI_STATUSLEDPIN 16
//connection status led enabled
#define WIFI_STATUSLEDENABLED 1
//serial enabled
#define WIFI_SERIALENABLED SERIAL_ENABLED
//do not block on connection and captive portal
#define WIFI_SKIPIFNOTCONNECTED 1
//do not block on connection and captive portal timeout (seconds)
#define WIFI_SKIPIFNOTCONNECTEDTIME 180
//check connection at selected interval (milliseconds)
#define WIFI_CHECKCONNECTIONINTERVAL 10000
//check connection status enabled
#define WIFI_CHECKCONNECTIONENABLED 1
//captive portal by button pin
#define WIFI_CAPTIVEPORTALBUTTONPIN 4
//captive portal by button is enabled
#define WIFI_CAPTIVEPORTALBUTTONENABLED 0
//hozone
#define WIFI_HOSTNAME "esplsiwindmeter"
//hostname length
#define WIFI_HOSTNAMELENGTH 15
//captive portal name
#define WIFI_CAPTIVEPORTALNAME "ESPLSIWindMeter-AP"

//get data interval ms
#define GETDATA_INTERVALMS 1000

//max interval
#define DISPLAY_MAX 99999999

//ADS1X15 address
#define ADS1115_ADDRESS 0x48
//ADS1X15 gain
#define ADS1115_GAIN GAIN_TWOTHIRDS
//ADS1X15 mV per bit
//ex. 16 bit on +/- 6.144V = 15bit on 6.144V
//    6.144V*2 / 2^16 = 0.0001875V
#define ADS1115_MVSTEP 0.1875

//LSI sensor ADC channel
#define LSI_ADCCHANNEL 0
//LSI sensor current to voltage convertion min volt
#define LSI_CTVMINVOLT 0
//LSI sensor current to voltage convertion max volt
#define LSI_CTVMAXVOLT 5
//LSI sensor current to voltage convertion min mamp
#define LSI_CTVMINMAMP 4
//LSI sensor current to voltage convertion max mamp
#define LSI_CTVMAXMAMP 20
//LSI sensor current to winds speed conversion
#define LSI_WINDSPEEDFORMULA(x) (50.0/16.0) * (x - 4.0)

//LSI sensor current max wind speed value
#define LSI_MAXVALUE 50.0
//LSI sensor current min wind speed value
#define LSI_MINVALUE 0.0

//adc ema filter alpha
#define ADC_EMAFILTERALPHA 50

//send data to thingspeak interval ms
#define THINGSPEAK_SENDDATAINTERVALMS 60000
//connection status led
#define THINGSPEAK_STATUSLEDPIN 14

//set MAX7219 7segment data pin
#define LED7SEG_DATAPIN 15
//set MAX7219 7segment clk pin
#define LED7SEG_CLKPIN 13
//set MAX7219 7segment cs pin
#define LED7SEG_CSPIN 12


//functions
extern String getLastdata();

#endif
