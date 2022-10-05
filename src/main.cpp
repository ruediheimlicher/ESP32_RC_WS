/**
 * ----------------------------------------------------------------------------
 * ESP32 Remote Control with WebSocket
 * ----------------------------------------------------------------------------
 * © 2020 Stéphane Calderoni
 * ----------------------------------------------------------------------------
 * https://rntlab.com/question/full-tutorial-esp32-remote-control-with-websocket/
 * ESP32/ESP8266: Control Outputs with Web Server and a Physical Button Simultaneously
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------

#define LED_PIN 26
#define BTN_PIN 22

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// Button debouncing
const uint8_t DEBOUNCE_DELAY = 10; // in milliseconds

// WiFi credentials
const char *WIFI_SSID = "AP RE";
const char *WIFI_PASS = "eiramsor44wl";

// ----------------------------------------------------------------------------
// Definition of the LED component
// ----------------------------------------------------------------------------

struct Led {
    // state variables
    uint8_t pin;
    bool    on;

    // methods
    void update() {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};

// ----------------------------------------------------------------------------
// Definition of the Button component
// ----------------------------------------------------------------------------

struct Button {
    // state variables
    uint8_t  pin;
    bool     lastReading;
    uint32_t lastDebounceTime;
    uint16_t state;

    // methods determining the logical state of the button
    bool pressed()                { return state == 1; }
    bool released()               { return state == 0xffff; }
    bool held(uint16_t count = 0) { return state > 1 + count && state < 0xffff; }

    // method for reading the physical state of the button
    void read() {
        // reads the voltage on the pin connected to the button
        bool reading = digitalRead(pin);

        // if the logic level has changed since the last reading,
        // we reset the timer which counts down the necessary time
        // beyond which we can consider that the bouncing effect
        // has passed.
        if (reading != lastReading) {
            lastDebounceTime = millis();
        }

        // from the moment we're out of the bouncing phase
        // the actual status of the button can be determined
        if (millis() - lastDebounceTime > DEBOUNCE_DELAY) {
            // don't forget that the read pin is pulled-up
            bool pressed = reading == LOW;
            if (pressed) {
                     if (state  < 0xfffe) state++;
                else if (state == 0xfffe) state = 2;
            } else if (state) {
                state = state == 0xffff ? 0 : 0xffff;
            }
        }

        // finally, each new reading is saved
        lastReading = reading;
    }
};

// ----------------------------------------------------------------------------
// Definition of global variables
// ----------------------------------------------------------------------------

Led    onboard_led = { LED_BUILTIN, false };
Led    led    = { LED_PIN, false };
Button button = { BTN_PIN, HIGH, 0, 0 };


// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initSPIFFS() {
  if (!SPIFFS.begin()) 
  {
    Serial.println("Cannot mount SPIFFS volume...");
    while (1) 
    {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }  
  }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Trying to connect [%s] ", WiFi.macAddress().c_str());
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.printf(" %s\n", WiFi.localIP().toString().c_str());
}


// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup() {
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(led.pin,         OUTPUT);
    pinMode(button.pin,      INPUT);

    Serial.begin(115200); delay(500);

    initSPIFFS();
    initWiFi();
}
// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop() {
    button.read();

    if (button.pressed()) led.on = !led.on;
    onboard_led.on = millis() % 1000 < 50;

    led.update();
    onboard_led.update();
}