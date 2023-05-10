#include "config.h"

int relayPin = 4; //defines the pin for the Non-Latching Relay FeatherWing
int relayState; //defines the state of the relay

// set up the 'fok' feed
AdafruitIO_Feed *fok = io.feed("fingerprint-ok");
AdafruitIO_Feed *door_lock = io.feed("Door lock"); // set up the 'Door lock' feed

void handleMessage(AdafruitIO_Data *data);

void setup() {
  pinMode(relayPin, OUTPUT); //initializes the relayPin as an output
// start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);

  Serial.println(WIFI_SSID);

  // start MQTT connection to io.adafruit.com
  io.connect();

  // set up a message handler for the count feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  fok->onMessage(handleMessage);

  // wait for an MQTT connection
  // NOTE: when blending the HTTP and MQTT API, always use the mqttStatus
  // method to check on MQTT connection status specifically
  while(io.mqttStatus() < AIO_CONNECTED) {
    Serial.print(io.statusText());
    delay(500);
    blinkLED(1,100);
  }

  fok->get();

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  blinkLED(3,50);

}

void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  // Because this sketch isn't publishing, we don't need
  // a delay() in the main program loop.
}

// this function is called whenever a 'fok' message
// is received from Adafruit IO. it was attached to
// the fok feed in the setup() function above.

void blinkLED(int reps, int delayTime) {
  for(int i=0; i<reps; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayTime);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayTime);
  }
}
void handleMessage(AdafruitIO_Data *data) {
  blinkLED(2,50);
  Serial.print("received <- ");
  Serial.println(data->value());
  if(data->toPinLevel() == HIGH) {
    digitalWrite(relayPin, HIGH);
    door_lock->save(true); // sends a value to the feed to open the lock
  }
  else {
    digitalWrite(relayPin, LOW);
    door_lock->save(false); // sends a value to the feed to close the lock
  }

}