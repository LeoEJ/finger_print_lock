#include "config.h"

// Fingerprint Sensor
#include <Adafruit_Fingerprint.h>

// OLED display
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <SPI.h>
#include <Wire.h>

// Getting time & date
#include "time.h"
#include "sntp.h"

AdafruitIO_Feed *fingerprint_ok = io.feed("Fingerprint OK"); // set up the 'Fingerprint OK' feed
AdafruitIO_Feed *door_lock = io.feed("Door lock"); // set up the 'Door lock' feed

#define mySerial Serial1

// define the pins for the buttons
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14
#define WIRE Wire

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial); // Initialize fingerprint sensor with mySerial connection
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire); // Initialize OLED display with width 64, height 128, and Wire connection
uint8_t id; // Declare an unsigned 8-bit integer variable named "id"

bool ballAnimationOn = true; // starts the bouncing ball animation
int ballX = 20;      // X coordinate of the ball's center
int ballY = 20;      // Y coordinate of the ball's center
int ballSize = 10;   // Diameter of the ball
int ballSpeedX = 2;  // Horizontal speed of the ball
int ballSpeedY = 1;  // Vertical speed of the ball
int gravity = 1;     // Acceleration due to gravity

// getting the time & date
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

// prints the current time & date on the OLED display
void printLocalTime()
{
  display.clearDisplay();
  display.setCursor(5,5);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    display.println("No time available (yet)");
    return;
  }
  display.setCursor(17,5);
  display.println(&timeinfo, "%A, %B %d");
  display.setCursor(50, 20);
  display.println(&timeinfo, "%Y");
  display.setCursor(35, 35);
  display.setTextSize(2);
  display.println(&timeinfo, "%H:%M");
  display.display();
  delay(5000);
  ballAnimationOn = true;
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  display.clearDisplay();
  display.setCursor(5,5);
  display.println("Got time adjustment from NTP!");
  display.display();
  printLocalTime();
}

void setup() {
  Serial.begin(115200);
  delay(250); // wait for the OLED to power up
  //while(!Serial);  // For Yun/Leo/Micro/Zero/...
  Serial.println("128x64 OLED FeatherWing test");
  display.begin(0x3C, true); // Address 0x3C default

  Serial.println("OLED begun");

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  display.setRotation(1);
  Serial.println("Button test");

  // initializes the buttons as inputs
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(5,5);
  display.println("Adafruit finger \n detect test");
  display.display(); // actually display all of the above

  finger.begin(57600);// set the data rate for the sensor serial port
  delay(5);

  // checks if the fingerprint sensor is found
  if (finger.verifyPassword()) {
    display.clearDisplay();
    display.setCursor(5,5);
    display.println("Found fingerprint sensor!");
    display.display();
  } else {
    display.clearDisplay();
    display.setCursor(5,5);
    display.println("Did not find fingerprint sensor :(");
    display.display();
    while (1) { delay(1); }
  }

  delay(100);
  Serial.println("\n\nAdafruit finger detect test");
  io.connect();
  while(io.status() < AIO_CONNECTED) {
    Serial.print(io.statusText());
    delay(500);
  }

  // This code block retrieves and displays various sensor parameters.
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }

  /**
   * NTP server address could be aquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE aquired NTP server address
   */
  sntp_servermode_dhcp(1);    // (optional)

  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagicaly.
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  //connect to WiFi
  Serial.printf("Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
}

// This function reads a number from the serial monitor and returns it as a uint8_t
uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop() {  // run over and over again   
  display.clearDisplay();
  // Draw ball
  if (ballAnimationOn) {
    display.fillCircle(ballX, ballY, ballSize, SH110X_WHITE);

    // Update ball position
    ballX += ballSpeedX;
    ballY += ballSpeedY;
    ballSpeedY += gravity;

    // Bounce ball off edges
    if (ballX + ballSize > display.width()) {
      ballX = display.width() - ballSize;
      ballSpeedX = -ballSpeedX;
    } else if (ballX - ballSize < 0) {
      ballX = ballSize;
      ballSpeedX = -ballSpeedX;
    }
    if (ballY + ballSize > display.height()) {
      ballY = display.height() - ballSize;
      ballSpeedY = -ballSpeedY;
    } else if (ballY - ballSize < 0) {
      ballY = ballSize;
      ballSpeedY = -ballSpeedY;
    }
  }

  // Display changes
  display.display();
  delay(20);

  io.run();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(5,5);
  long now = millis();
  if (!digitalRead(BUTTON_A)) { // button A is pressed enroll
    ballAnimationOn = false; // disable the ball animation
    display.clearDisplay();
    display.print("Please scan an authorized fingerprint");
    display.display();
    while((millis()-now) < 5000){ // try for 5 seconds
      if (finger.getImage() == FINGERPRINT_OK) { // if the sensor succesfully captured an image of the fingerprint
        if (finger.image2Tz() == FINGERPRINT_OK){ // if the sensor succesfully converted the image to a template
          if (finger.fingerSearch() == FINGERPRINT_OK){ // if the sensor found a match
            if(finger.fingerID == 1 || finger.fingerID == 2 || finger.fingerID == 3){ // if the fingerprint is any of these three
            enroll(); // run the enroll function
            }
            else{
              display.clearDisplay();
              display.setCursor(5,5);
              display.print("Fingerprint not apporved");
              display.display();
              delay(4000);
              ballAnimationOn = true;
            }
          }
        } 
      }
      else{
        ballAnimationOn = true;
      }
    }
  }
  
  if(!digitalRead(BUTTON_B)) { // button B is pressed
    ballAnimationOn = false;
    display.clearDisplay();
    display.print("Please scan an authorized fingerprint");
    display.display();
    while((millis()-now) < 5000){ // try for 5 seconds
      if (finger.getImage() == FINGERPRINT_OK) {
        if (finger.image2Tz() == FINGERPRINT_OK){
          if (finger.fingerSearch() == FINGERPRINT_OK){
            if(finger.fingerID == 1 || finger.fingerID == 2 || finger.fingerID == 3){
            remove();
            }
            else{
              display.clearDisplay();
              display.setCursor(5,5);
              display.print("Fingerprint not apporved");
              display.display();
              delay(4000);
              ballAnimationOn = true;
            }
          }
        } 
      }
      else{
        ballAnimationOn = true;
      }
    }
  }

  if (!digitalRead(BUTTON_C)) { //button C is pressed
    ballAnimationOn = false;
    display.clearDisplay();
    printLocalTime();
  }
  getFingerprintID(); // run the getFingerprintID() function
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch(); // searching for a fingerprint
  Serial.println(p);
  if (p == FINGERPRINT_OK) { // if a fingerprint was found
    Serial.println("Found a match!");
    Serial.print("sending value -> ");
    fingerprint_ok->save(true); // sends a value to the feed to open the lock
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(5,5);
    Serial.println("received");
    if(finger.fingerID == 1){ // the fingerprint stored in the first template was found
      display.clearDisplay();
      display.print(" Welcome \n  Yossef!");
      display.setTextSize(1);
      display.setCursor(20, 45);
      display.print("Door Unlocked");
    }
    else if(finger.fingerID == 2){ // the fingerprint stored in the second template was found
      display.clearDisplay();
      display.print(" Welcome \n Leonardo!");
      display.setTextSize(1);
      display.setCursor(20, 45);
      display.print("Door Unlocked");
    }
    else if(finger.fingerID == 3){ // the fingerprint stored in the third template was found
      display.clearDisplay();
      display.print(" Welcome \n Axel!");
      display.setTextSize(1);
      display.setCursor(20, 45);
      display.print("Door Unlocked");
    }
    else {
      display.clearDisplay();
      display.print("Welcome!");
      display.setTextSize(1);
      display.setCursor(20, 45);
      display.print("Door Unlocked");
    }
    display.display();
    io.run();
    delay(30000);
    fingerprint_ok->save(false); // sends a value to close the lock
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(5,5);
    display.print("Did not find a match");
    display.display();
    delay(4000);
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

void enroll(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(5,5);
  display.clearDisplay();
  display.println("Ready to enroll a fingerprint!");
  display.print("\n");
  display.println("Please type in the ID # (from 1 to 127) \nyou want to save this finger as...");
  display.display();
  id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }
  display.clearDisplay();
  display.setCursor(5,5);
  display.print("Enrolling ID #");
  display.println(id);
  display.display();

  while (!  getFingerprintEnroll() );
}

void remove(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(5,5);
  display.println("Please type in the ID # (from 1 to 127) you want to delete...");
  display.display();
  uint8_t id = readnumber();
  if (id == 0) {// ID #0 not allowed, try again!
     return;
  }

  display.clearDisplay();
  display.print("Deleting ID #");
  display.println(id);
  display.display();

  deleteFingerprint(id);
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(5,5);
  display.print("Waiting for valid finger to enroll as #"); display.println(id);
  display.display();
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Image taken");
      display.display();
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Communication error");
      display.display();
      break;
    case FINGERPRINT_IMAGEFAIL:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Imaging error");
      display.display();
      break;
    default:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Unknown error");
      display.display();
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Image converted");
      display.display();
      break;
    case FINGERPRINT_IMAGEMESS:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Image too messy");
      display.display();
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Communication error");
      display.display();
      return p;
    case FINGERPRINT_FEATUREFAIL:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Could not find fingerprint features");
      display.display();
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Could not find fingerprint features");
      display.display();
      return p;
    default:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Unknown error");
      display.display();
      return p;
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Remove finger");
  display.display();
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  display.clearDisplay();
  display.print("ID "); display.println(id);
  p = -1;
  display.setCursor(0,0);
  display.println("Place same finger again");
  display.display();
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      display.setCursor(0,0);
      display.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      display.setCursor(0,0);
      display.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      display.setCursor(0,0);
      display.println("Imaging error");
      break;
    default:
      display.setCursor(0,0);
      display.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("Image converted");
      display.display();
      break;
    case FINGERPRINT_IMAGEMESS:
      display.setCursor(0,0);
      display.println("Image too messy");
      display.display();
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      display.setCursor(0,0);
      display.println("Communication error");
      display.display();
      return p;
    case FINGERPRINT_FEATUREFAIL:
      display.setCursor(0,0);
      display.println("Could not find fingerprint features");
      display.display();
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      display.setCursor(0,0);
      display.println("Could not find fingerprint features");
      display.display();
      return p;
    default:
      display.setCursor(0,0);
      display.println("Unknown error");
      display.display();
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Prints matched!");
    display.display();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Communication error");
    display.display();
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Fingerprints did not match");
    display.display();
    return p;
  } else {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Unknown error");
    display.display();
    return p;
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("ID "); display.println(id);
  display.display();
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Stored!");
    display.display();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Communication error");
    display.display();
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Could not store in that location");
    display.display();
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Error writing to flash");
    display.display();
    return p;
  } else {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Unknown error");
    display.display();
    return p;
  }

  return true;
}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Deleted!");
    display.display();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Communication error");
    display.display();
  } else if (p == FINGERPRINT_BADLOCATION) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Could not delete in that location");
    display.display();
  } else if (p == FINGERPRINT_FLASHERR) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Error writing to flash");
    display.display();
  } else {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Unknown error: 0x"); Serial.println(p, HEX);
    display.display();
  }

  return p;
}
