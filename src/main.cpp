/*
ESP LSI Wind Meter v01

copyright (c) Davide Gironi, 2020

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#include <Arduino.h>

#include <Wire.h>

//include main
#include "main.h"

//WiFiManager helper library
#include "WiFiManagerHelper/WiFiManagerHelper.h"
//initialize WiFiManagerHelper
WiFiManagerHelper wifiManagerHelper;

//LedControl library
#include "LedControl.h"
//initialize LedControl
LedControl ledControl = LedControl(LED7SEG_DATAPIN, LED7SEG_CLKPIN, LED7SEG_CSPIN, 1);

//include pages
#include "page.h"

//include eepromConfig
#include "eepromconfig.h"
extern struct EEPROMConfigStruct eepromConfig;

//HTTP client library
#include <ESP8266HTTPClient.h>

//ADS1X15 library
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads(ADS1115_ADDRESS);

//ThingSpeak library
#include "ThingSpeak.h"

//check getdata interval time
static unsigned long getdataLast = 0;

//check senddata interval time
static unsigned long senddataLast = 0;

//last written data
static double lastdatawindlsi = 0;

// Initialize the client library
WiFiClient client;


/**
 * Get last data
 */
String getLastdata() {
  String ret = "";

  //get data
  char numberc[20];
  sprintf(numberc, "Wind speed: <b>%0.2f m/s</b>", lastdatawindlsi);
  ret = ret + numberc;
  Serial.println(ret);

  return ret;
}

/**
 * Exponential Moving Avarage filter
 */
unsigned int adcEmafilter(unsigned int newvalue, unsigned int value)
{
	//use exponential moving avarate Y=(1-alpha)*Y + alpha*Ynew, alpha between 1 and 0
	//in uM we use int math, so Y=(63-63alpha)*Y + 63alpha*Ynew  and  Y=Y/63 (Y=Y>>6)
	value = (64-ADC_EMAFILTERALPHA)*value+ADC_EMAFILTERALPHA*newvalue;
	value = (value>>6);
	return value;
}

/**
 * Print out to display
 */
void setDisplayGen(String stringdisp, uint8_t isnumber, uint8_t dotindex) {
  static String stringdisplast = "";

  //only print if string differs
  if(stringdisp == stringdisplast)
    return;
  else {
#if SERIAL_ENABLED == 1
    Serial.print("Display ");
    Serial.println(stringdisp);
#endif
    stringdisplast = stringdisp;
    ledControl.clearDisplay(0);
  }

  //write out string
  uint8_t i = 0;
  for(i=0; i<8; i++) {
    if(dotindex == stringdisp.length() - 1 - i + 1) {
      ledControl.setChar(0, i, stringdisp.charAt(stringdisp.length() - 1 - i), 1);
    } else
      ledControl.setChar(0, i, stringdisp.charAt(stringdisp.length() - 1 - i), 0);
    if(i+1 > (int)stringdisp.length())
      ledControl.setChar(0, i, ' ', 0);
  }
}

/**
 * Print out a text display
 */
void setDisplayText(String text) {
  //truncate string
  if(text.length() > 8)
    text = text.substring(0, 8);
  
  //display the text
  setDisplayGen(text, 0, 255);
}

/**
 * Print out a double to display
 */
void setDisplayDouble(double number) {
  //sanitize number and build the string
  char numberc[10];
  if(number > 99999999) {
    number = 99999999;
    sprintf(numberc, "%0.0f", number);
  } else if(number > 9999999) {
    sprintf(numberc, "%0.0f", number);
  } else if(number > 999999) {
    sprintf(numberc, "%0.1f", number);
  } else if(number < -9999999) {
    number = -9999999;
    sprintf(numberc, "%0.0f", number);
  } else if(number < -999999) {
    sprintf(numberc, "%0.0f", number);
  } else if(number < -99999) {
    sprintf(numberc, "%0.1f", number);
  } else {
    sprintf(numberc, "%0.2f", number);
  }
  String numbert = numberc;
  
  //find the dot
  uint8_t dotindex = numbert.indexOf('.');
  if(dotindex != 255) {
    numbert.remove(dotindex, 1);
  }

  //display the number
  setDisplayGen(numbert, 1, dotindex);
}

/**
 * Get data and set display numbers
 */
void getData() {
  static int16_t adclsi = 0;

#if SERIAL_ENABLED == 1
  Serial.println("Get data");
#endif

  //get adc for LSI sensor
  int16_t adclsit = ads.readADC_SingleEnded(LSI_ADCCHANNEL);
  //filter adc for LSI sensor
  adclsi = adcEmafilter(adclsit, adclsi);
  //get voltage for LSI sensor
  int16_t mvoltlsi = adclsi * ADS1115_MVSTEP;
  //linear interpolate volts to amp
  int16_t uamplsi = (int16_t)map(mvoltlsi, LSI_CTVMINVOLT*1000, LSI_CTVMAXVOLT*1000, LSI_CTVMINMAMP*1000, LSI_CTVMAXMAMP*1000);
  if(uamplsi > LSI_CTVMAXMAMP*1000)
    uamplsi = LSI_CTVMAXMAMP*1000;
  if(uamplsi < LSI_CTVMINMAMP*1000)
    uamplsi = LSI_CTVMINMAMP*1000;
  //transform to mamp and round to 2 decimal digits
  double mamplsi = round((uamplsi/1000.0)*100)/100.0;
  //get wind speed
  double windlsi = LSI_WINDSPEEDFORMULA(mamplsi);
  if(windlsi > LSI_MAXVALUE)
    windlsi = LSI_MAXVALUE;
  if(windlsi < LSI_MINVALUE)
    windlsi = LSI_MINVALUE;
  //round to 1 decimal digits
  windlsi = round(windlsi*10)/10.0;

#if SERIAL_ENABLED == 1
#if SERIALDEBUGSENSOR_ENABLED == 1
  Serial.print("LSI ADC: "); Serial.println(adclsi);
  Serial.print("LSI mVOLT: "); Serial.println(mvoltlsi);
  Serial.print("LSI mAMP: "); Serial.println(mamplsi);
  Serial.print("LSI Wind: "); Serial.println(windlsi);
#endif
#endif

  setDisplayDouble(windlsi);

  //update last data
  lastdatawindlsi = windlsi;
}

/**
 * Send data to ThingSpeak
 */
void sendDataToThingSpeak() {
#if SERIAL_ENABLED == 1
  Serial.print("Sending data to ThingSpeak...");
#endif

  int httpCode = ThingSpeak.writeField(atol(eepromConfig.thingspeakchannelid), 1, (float)lastdatawindlsi, eepromConfig.thingspeakapikey);
  if (httpCode == 200) {
#if SERIAL_ENABLED == 1
    Serial.println("Succesfully sent");
#endif
    digitalWrite(THINGSPEAK_STATUSLEDPIN, HIGH);
  } else {
#if SERIAL_ENABLED == 1
    Serial.println("Sent with error " + String(httpCode));
#endif
    digitalWrite(THINGSPEAK_STATUSLEDPIN, LOW);
  }
}

/**
 * Main setup
 */
void setup() {
#if SERIAL_ENABLED == 1
  //initialize Serial
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting...");
#endif

  //initialize eeprom
#if SERIAL_ENABLED == 1
  Serial.println("Loading eeprom");
#endif
  eepromInit();
  eepromRead();

  //initilize 7seg
  ledControl.shutdown(0, false);
  ledControl.setIntensity(0, 8);
  ledControl.clearDisplay(0);
  
  //initizlied adc
#if SERIAL_ENABLED == 1
  Serial.println("Initialize ADS1X15 ADC");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV)");
#endif
  ads.begin();
  ads.setGain(ADS1115_GAIN);

  //set the string to display
  setDisplayText("con...");

  //set WiFi
#if SERIAL_ENABLED == 1
  Serial.println("Set the WiFi");
#endif
  char *apname = (char *)WIFI_CAPTIVEPORTALNAME;
  char deviceidc[4];
  snprintf(deviceidc, sizeof(deviceidc), "%03d", eepromConfig.deviceid);
  char hostname [WIFI_HOSTNAMELENGTH + 4];
  strcpy(hostname, WIFI_HOSTNAME);
  strcat(hostname, deviceidc);
  //set the connection manager
  wifiManagerHelper.setSerialDebug(WIFI_SERIALENABLED);
  wifiManagerHelper.setCheckConnection(WIFI_CHECKCONNECTIONINTERVAL, WIFI_CHECKCONNECTIONENABLED);
  wifiManagerHelper.setStatusLed(WIFI_STATUSLEDPIN, WIFI_STATUSLEDENABLED);
  wifiManagerHelper.setCaptivePortalOnButton(WIFI_CAPTIVEPORTALBUTTONPIN, WIFI_CAPTIVEPORTALBUTTONENABLED);
  //try to connect
  wifiManagerHelper.connect(hostname, apname, WIFI_SKIPIFNOTCONNECTEDTIME, WIFI_SKIPIFNOTCONNECTED);
#if SERIAL_ENABLED == 1
  if(wifiManagerHelper.isConnected()) {
    Serial.println("Status: connected");
    Serial.print("    IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Status: not connected");
  }
#endif

  //disply the current ip
  if(wifiManagerHelper.isConnected()) {
    setDisplayText(String(WiFi.localIP()[0]));
    delay(2000);
    setDisplayText(String(WiFi.localIP()[1]));
    delay(2000);
    setDisplayText(String(WiFi.localIP()[2]));
    delay(2000);
    setDisplayText(String(WiFi.localIP()[3]));
    delay(2000);
    setDisplayText("");
  }

  //initialize webserver
#if SERIAL_ENABLED == 1
  Serial.println("Starting webserver");
#endif
  pageInit();

  //initialize thingspeak
  ThingSpeak.begin(client);
  pinMode(THINGSPEAK_STATUSLEDPIN, OUTPUT);
  digitalWrite(THINGSPEAK_STATUSLEDPIN, LOW);

  //initialize last time
  getdataLast = millis();
  senddataLast = millis();
}

/**
 * Main loop
 */
void loop() {
  //check connection
  wifiManagerHelper.checkConnection();

  //page server handler
  pageHandleClient();

  //get data
  if((millis() - getdataLast) > GETDATA_INTERVALMS) {
    getdataLast = millis();
    getData();
  }
  
  //send to thingspeak
  if(wifiManagerHelper.isConnected()) {
    if((millis() - senddataLast) > THINGSPEAK_SENDDATAINTERVALMS) {
      senddataLast = millis();
      sendDataToThingSpeak();
    }
  }
}
