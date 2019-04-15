//display connect ip address
//connect to network
//set time from NTP
//get the time from the clock
//display time and time since coffee brewed
//send messge to slack when brewed
//send the time coffee was brewed to GitHub for storage?
//reset NTP every 6 hours if it is available
//TODO: OTA updates
//TODO: SECURITY
#include "config.h"
//const String slack_hook_url = "https://hooks.slack.com/services/1234567

#include <Ticker.h>
Ticker displayer;
const int buttonPin = D3;    // the number of the pushbutton pin constant
int buttonState = HIGH;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin

/*
 SLACK CONFIGURATION
 */


const String slack_message = "A rockstar made coffee!!!";
const String slack_username = "BunnBot";
const String icon_emoji = ":coffee:";





#include <ESP8266WiFi.h>
//WIFI MANAGER
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager0


//WiFiClient client;
// or... use WiFiFlientSecure for SSL
WiFiClientSecure client;


#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
//SDA>D2 AND SCL>D1
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#include <SNTPtime.h>
SNTPtime NTPch("ca.pool.ntp.org");
strDateTime dateTime;
strDateTime madeTimeF;
unsigned long madeTime;
unsigned long  elapsed;

#include <Timezone.h>
//TimeZone
TimeChangeRule myDST = {"PDT", Second, Sun, Mar, 2, -420};    //Daylight time = UTC - 7 hours
TimeChangeRule mySTD = {"PST", First, Sun, Nov, 2, -480};     //Standard time = UTC - 8 hours
Timezone myTZ(myDST, mySTD);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;



byte actualHour = dateTime.hour;
byte actualMinute = dateTime.minute;
byte actualSecond = dateTime.second;
int actualYear = dateTime.year;
byte actualMonth = dateTime.month;
byte actualDay = dateTime.day;
byte actualDayofWeek = dateTime.dayofWeek;

boolean daylight = dateTime.valid; 

void LCDdisplay()
{

  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.print(dateTime.hour );
  lcd.print(":");
  if (dateTime.minute < 10) {
    lcd.print("0");
  }
  lcd.print(dateTime.minute);
  lcd.setCursor(5, 0);
  lcd.print(" Brew:");
  if (madeTimeF.hour) {
    lcd.print(madeTimeF.hour);
    lcd.print(":");
    if (madeTimeF.minute < 10) {
      lcd.print("0");
    }
    lcd.print(madeTimeF.minute);
  }

  lcd.setCursor(0, 1);
  elapsed = (time(NULL) - madeTime) / 60;

  lcd.print("Elapsed:");
  lcd.print(elapsed);
  lcd.setCursor(13, 1);
  lcd.print("min");
  lcd.display();

} //end LCDdisplay()


bool postMessageToSlack(String msg)
{
  const char* host = "hooks.slack.com";
  const char* fingerprint = "‎3b f8 1c a7 74 a8 6b c5 d8 95 fe e2 2e de 65 a1";
  
  Serial.print("Connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpsPort = 443;
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed :-(");
    return false;
  }

  // We now create a URI for the request

  Serial.print("Posting to URL: ");
  Serial.println(slack_hook_url);

  String postData="payload={\"link_names\": 1, \"icon_emoji\": \"" + icon_emoji + "\", \"username\": \"" + slack_username + "\", \"text\": \"" + msg + "\"}";
  //String postData="payload={\"link_names\": 1, \"icon_url\": \"" + slack_icon_url + "\", \"username\": \"" + slack_username + "\", \"text\": \"" + msg + "\"}";
  // This will send the request to the server
  client.print(String("POST ") + slack_hook_url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Connection: close" + "\r\n" +
               "Content-Length:" + postData.length() + "\r\n" +
               "\r\n" + postData);
  Serial.println("Request sent");
  String line = client.readStringUntil('\n');
  Serial.printf("Response code was: ");
  Serial.println(line);
  if (line.startsWith("HTTP/1.1 200 OK")) {
    return true;
  } else {
    return false;
  }
}


void setup()
{
  int error;
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
 

  //WiFiManager
  //Local initialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
  //reset saved settings
  //wifiManager.resetSettings();

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "coffeeTime"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("coffeeTime");



  while (!NTPch.setSNTPtime()) Serial.print("."); // set internal clock
  Serial.println();
  Serial.println("Time set");
  dateTime = NTPch.getTime(-8.0, 2); // get time from internal clock
  NTPch.printDateTime(dateTime);

  Serial.println("LCD...");
  while (! Serial);
  Serial.println("checking  LCD");
  // See http://playground.arduino.cc/Main/I2cScanner
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.print(error);
  if (error == 0) {
    Serial.println(": LCD found.");
  } else {
    Serial.println(": LCD not found.");
  } // if
  lcd.begin(16, 2); // initialize the lcd
  lcd.setBacklight(155);
  lcd.home();

  displayer.attach(1, LCDdisplay);

} // setup()


void loop()
{
  dateTime = NTPch.getTime(-8.0, 2); // get time from internal clock
  // first parameter: Time zone; second parameter: 1 for European summer time; 2 for US daylight saving time (not implemented yet)


  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);
  Serial.print("reading:");
  Serial.println(reading);

  // if the button state has changed:
  Serial.print("buttonState:");
  Serial.println(buttonState);

  if (reading != buttonState) {
    buttonState = reading;
    // log time button pushed
    if (buttonState == HIGH) {
      madeTime = time(NULL);
      madeTimeF = NTPch.getTime(-8.0, 1); // get time from internal clock
      postMessageToSlack(slack_message) ;
      Serial.print("button pressed");
      Serial.println(madeTime);
    }
  }

  delay(200);
} // loop()
