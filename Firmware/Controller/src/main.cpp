#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define X_AXIS_PIN 32
#define Y_AXIS_PIN 33
#define LIGHT_PIN 25
#define STEER_RIGHT_PIN 14
#define STEER_LEFT_PIN 27

// Address for Battery Pack
uint8_t broadcastAddress[] = {0x40,0xF5,0x20,0x08,0xE3,0x8D};  //40:F5:20:08:E3:8D - Proto Board

//Struct for data out
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
PacketData data;

//ints for storing x + y
int xrawAxisValue;
int yrawAxisValue;

esp_now_peer_info_t peerInfo;

/* This function is used to map 0-4095 joystick value to 0-254.
   We also set the deadzone
*/
int mapAndAdjustJoystickDeadBandValues(int value, bool reverse)
{
  if (value >= 2200)
  {
    value = map(value, 2200, 4095, 127, 254);
  }
  else if (value <= 1800)
  {
    value = map(value, 1800, 0, 127, 0);  
  }
  else
  {
    value = 127;
  }

  if (reverse)
  {
    value = 254 - value;
  }
  return value;
}

// Data sen ok?
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t ");
  Serial.println(status);
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Message sent" : "Message failed");
}

void setup() 
{
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else
  {
    Serial.println("Succes: Initialized ESP-NOW");
  }

  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  else
  {
    Serial.println("Succes: Added peer");
  } 

  //Configure pin modes
  pinMode(LIGHT_PIN, INPUT_PULLUP); 
  pinMode(STEER_LEFT_PIN, INPUT_PULLUP); 
  pinMode(STEER_RIGHT_PIN, INPUT_PULLUP); 

}
 
void loop() 
{
  //Basic light values for testing
  data.rLedValue = 255;
  data.gLedValue = 255;
  data.bLedValue = 255;


  //Preconfigure the data for the send package.
  data.xAxisValue = mapAndAdjustJoystickDeadBandValues(analogRead(X_AXIS_PIN), false);
  data.yAxisValue = mapAndAdjustJoystickDeadBandValues(analogRead(Y_AXIS_PIN), false);  
  data.steerLeft = false;
  data.steerRight = false;
  data.lightValue = false;

  //Read the joystick and print it out
  xrawAxisValue= analogRead(X_AXIS_PIN);
  yrawAxisValue= analogRead(Y_AXIS_PIN); 
  Serial.print ("X Raw ");
  Serial.println (xrawAxisValue);
  Serial.print ("Y Raw ");
  Serial.println (yrawAxisValue);


  //Read inputs and configure values for data out
  if (digitalRead(LIGHT_PIN) == LOW)
  {
    data.lightValue = true;
  }

  if (digitalRead(STEER_LEFT_PIN) == LOW)
  {
    data.steerLeft = true;
  }

  if (digitalRead(STEER_RIGHT_PIN) == LOW)
  {
    data.steerRight = true;
  }
  
  //Report send status
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));
  if (result == ESP_OK) 
  {
    Serial.println("Sent with success");
  }
  else 
  {
    //Someting wong
    Serial.println("Error sending the data");
  }    
  //Delay for debounce and light button.
  if (data.lightValue == true)
  {
    delay(50);
  }
  else
  {
    delay(50);
  }
}