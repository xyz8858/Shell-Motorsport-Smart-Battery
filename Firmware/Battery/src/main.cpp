#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_NeoPixel.h>

#define NUMPIXELS 2

//Pin definitions
//LEFT and RIGHT LED to become motor drive control.
const uint8_t DRIVE_1 = 13;
const uint8_t DRIVE_2 = 4;
const uint8_t STEERLEFT = 12;
const uint8_t STEERRIGHT = 5;
const uint8_t LED_CONTROL = 13;
const uint8_t STATUS_LED = 14;

Adafruit_NeoPixel pixels(NUMPIXELS, LED_CONTROL, NEO_GRB + NEO_KHZ800);

int currentLedState = HIGH;
int previousLedControlState = 0;

unsigned long startMillis;
unsigned long currentMillis;

ADC_MODE(ADC_VCC);

// Struct for the data coming from the controller
struct PacketData
{
  byte xAxisValue;
  byte yAxisValue;
  byte lightValue;
  byte steerLeft;
  byte steerRight;
  byte rLedValue;
  byte gLedValue;
  byte bLedValue;
};
PacketData receiverData;


/*
Control functionality using values from Controller.
Controller will send values from 0-255 for throttle position
"Turbo Mode" is taken care of in the PWM speed control. Full stick = 100% duty cycle
127-0 = Reverse speed control
127-255 = Forward speed control
Steering will be single buttons and digitaly written to the control pins
Lights are addressable - Code not written yet.
*/
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&receiverData, incomingData, sizeof(receiverData));
  String inputData ;
  inputData = inputData + "values " "X" + receiverData.xAxisValue + " Y " + receiverData.yAxisValue + " L " + receiverData.steerLeft + " R " + receiverData.steerRight + "L " + receiverData.lightValue;
  Serial.println(inputData);
  String lightData;
  lightData = lightData + "R= " + receiverData.rLedValue + "G= " + receiverData.gLedValue + "B= " + receiverData.bLedValue;
  Serial.println (lightData);

    // Mapping the data from a single 0-255 to match a Throttle and Brake int
    int forward = map( receiverData.xAxisValue, 127, 254, 0, 255);
    int reverse = map( receiverData.xAxisValue, 127, 0, 0, 255);

    int currentLedControlState;

    currentLedControlState = receiverData.lightValue;
    Serial.print(currentLedControlState);
    
    //Init comms watchdog timer
    startMillis = currentMillis;

   // Commented out as using neopixels.
   // digitalWrite(LED_CONTROL, currentLedState);

    // Control for neopixels - Need to check this command and wether it clears the Neos too quickly
    pixels.clear();

   if (currentLedState == LOW) {
     pixels.setPixelColor(1, pixels.Color(receiverData.rLedValue, receiverData.gLedValue, receiverData.bLedValue));
     pixels.setPixelColor(2, pixels.Color(receiverData.rLedValue, receiverData.gLedValue, receiverData.bLedValue));
     pixels.show();
    }

   if (currentLedState == HIGH) {
     pixels.setPixelColor(1, pixels.Color(0, 0, 0));
     pixels.setPixelColor(2, pixels.Color(0, 0, 0));
     pixels.show();
    }

  //Logic for deciding control and enacting the motors
  //Y axis unused
  if (receiverData.yAxisValue >= 129) {
    Serial.println("Y HIGH ");
  }
  if (receiverData.yAxisValue <= 125) {
    Serial.println("Y LOW ");
  }
  if (receiverData.xAxisValue >= 129) {
    Serial.println("X HIGH ");
    //Writes mapped value for forward
    analogWrite(DRIVE_1, forward);
    digitalWrite(DRIVE_2, LOW);
    digitalWrite(STATUS_LED, LOW);
  }
  if (receiverData.xAxisValue <= 125) {
    Serial.println("X LOW ");
    //Writes mapped value for reverse
    analogWrite(DRIVE_2, reverse);
    digitalWrite(DRIVE_1, LOW);
    digitalWrite(STATUS_LED, LOW);
  }
    //Stops drive motor when no input
    if (receiverData.xAxisValue >= 125 && receiverData.xAxisValue <= 129) {
    Serial.println("Drive Motor Stopped");
    //Writes mapped value for reverse
    digitalWrite(DRIVE_2, LOW);
    digitalWrite(DRIVE_1, LOW);
    digitalWrite(STATUS_LED, HIGH);
  }

  //Steering control
  if (receiverData.steerLeft == 1){
    //Steer left
    Serial.println("Steering Left");
    digitalWrite(STEERLEFT, HIGH);
    digitalWrite(STEERRIGHT, LOW);
    digitalWrite(STATUS_LED, LOW);
  }
 
  if (receiverData.steerRight == 1){
    //Steer right
    Serial.println("Steering Right");
    digitalWrite(STEERLEFT, LOW);
    digitalWrite(STEERRIGHT, HIGH);
    digitalWrite(STATUS_LED, LOW);
  }
   //Centers steering when not active 
   if (receiverData.steerRight == 0 && receiverData.steerLeft == 0){
    //Center
    Serial.println("Steering Center");
    digitalWrite(STEERLEFT, LOW);
    digitalWrite(STEERRIGHT, LOW);
    digitalWrite(STATUS_LED, HIGH);
  }

  //Light control
  if (previousLedControlState == 0 && currentLedControlState == 1) {
    Serial.print("Changing the LED state");
    currentLedState = !currentLedState;
  }

    Serial.print(previousLedControlState);
    previousLedControlState = currentLedControlState;
    
    Serial.println(ESP.getVcc()/1000.00);

}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(74880);

  delay(100);

  startMillis = millis();

  pixels.setBrightness(255);

  //Print device MAC address
  Serial.println("WIFI MAC ADDRESS ");
  Serial.println(WiFi.macAddress());
  
  //Setup control pins``  
  pinMode(DRIVE_1, OUTPUT);
  pinMode(DRIVE_2, OUTPUT);
  pinMode(STEERLEFT, OUTPUT);
  pinMode(STEERRIGHT, OUTPUT);
  pinMode(LED_CONTROL, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  //Default state for motors when not being driven
  digitalWrite(DRIVE_1, LOW);
  digitalWrite(DRIVE_2, LOW);
  digitalWrite(STEERLEFT, LOW);
  digitalWrite(STEERRIGHT, LOW);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
 //Bounce those mof**kin LEDs on bootup
  digitalWrite(STATUS_LED, LOW);
  delay(100);
  digitalWrite(STATUS_LED, HIGH);
  delay(100);
  digitalWrite(STATUS_LED, LOW);
  delay(100);
  digitalWrite(STATUS_LED, HIGH);
  delay(100);
  digitalWrite(STATUS_LED, LOW);
  delay(100);
  digitalWrite(STATUS_LED, HIGH);
  delay(100);
  digitalWrite(STATUS_LED, LOW);
  delay(100);
  digitalWrite(STATUS_LED, HIGH);
  delay(100);
  digitalWrite(STATUS_LED, LOW);
  delay(100);
  digitalWrite(STATUS_LED, HIGH);
  delay(100);
  digitalWrite(STATUS_LED, LOW);
  digitalWrite(STATUS_LED, HIGH);
  delay(100);

}

void loop() {
  //Motors off if no comms.
  //get the current time
  currentMillis = millis(); 
  //test whether the period has elapsed
  if (currentMillis - startMillis >= 500)  
  {
  Serial.println(currentMillis - startMillis);  
  digitalWrite(DRIVE_1, LOW);
  digitalWrite(DRIVE_2, LOW);
  digitalWrite(STEERLEFT, LOW);
  digitalWrite(STEERRIGHT, LOW);
  pixels.clear();
  startMillis = currentMillis;
  }
}
