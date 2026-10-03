#include <WiFi.h> 
#include <esp_now.h> 
#include <esp_wifi.h> 
 
// ===================================================== 
// JACKET MOTOR PINS 
// ===================================================== 
 
#define LEFT_MOTOR   25 
#define FRONT_MOTOR  26 
#define RIGHT_MOTOR  27 
#define BACK_MOTOR   14 
 
// ===================================================== 
// ESP-NOW MESSAGE 
// ===================================================== 
 
typedef struct struct_message 
{ 
  char command[20]; 
} struct_message; 
 
struct_message incomingMessage; 
 
// ===================================================== 
// MOTOR CONTROL 
// ===================================================== 
 
void allMotorsOff() 
{ 
  digitalWrite(LEFT_MOTOR, LOW); 
  digitalWrite(FRONT_MOTOR, LOW); 
  digitalWrite(RIGHT_MOTOR, LOW); 
  digitalWrite(BACK_MOTOR, LOW); 
} 
 
void leftMotor() 
{ 
  allMotorsOff(); 
  digitalWrite(LEFT_MOTOR, HIGH); 
} 
 
void frontMotor() 
{ 
  allMotorsOff(); 
  digitalWrite(FRONT_MOTOR, HIGH); 
} 
 
void rightMotor() 
{ 
  allMotorsOff(); 
  digitalWrite(RIGHT_MOTOR, HIGH); 
} 
 
void backMotor() 
{ 
  allMotorsOff(); 
  digitalWrite(BACK_MOTOR, HIGH); 
} 
 
void criticalAlert() 
{ 
  digitalWrite(LEFT_MOTOR, HIGH); 
  digitalWrite(FRONT_MOTOR, HIGH); 
  digitalWrite(RIGHT_MOTOR, HIGH); 
  digitalWrite(BACK_MOTOR, HIGH); 
} 
 
// ===================================================== 
// COMMAND HANDLER 
// ===================================================== 
 
void handleCommand(const char *command) 
{ 
  Serial.print("COMMAND RECEIVED: "); 
  Serial.println(command); 
 
  if (strcmp(command, "LEFT") == 0) 
  { 
    leftMotor(); 
  } 
  else if (strcmp(command, "FRONT") == 0) 
  { 
    frontMotor(); 
  } 
  else if (strcmp(command, "RIGHT") == 0) 
  { 
    rightMotor(); 
  } 
  else if (strcmp(command, "BACK") == 0) 
  { 
    backMotor(); 
  } 
  else if (strcmp(command, "CRITICAL") == 0) 
  { 
    criticalAlert(); 
  } 
  else if (strcmp(command, "STOP") == 0) 
  { 
    allMotorsOff(); 
  } 
  else 
  { 
    Serial.println("UNKNOWN COMMAND"); 
    allMotorsOff(); 
  } 
} 
 
// ===================================================== 
// ESP-NOW RECEIVE CALLBACK 
// ===================================================== 
 
void onDataRecv( 
  const esp_now_recv_info_t *info, 
  const uint8_t *data, 
  int len 
) 
{ 
  if (len <= 0) 
  { 
    return; 
  } 
 
  memset( 
    &incomingMessage, 
    0, 
    sizeof(incomingMessage) 
  ); 
 
  memcpy( 
    &incomingMessage, 
    data, 
    min( 
      len, 
      (int)sizeof(incomingMessage) 
    ) 
  ); 
 
  incomingMessage.command[ 
    sizeof(incomingMessage.command) - 1 
  ] = '\0'; 
 
  handleCommand( 
    incomingMessage.command 
  ); 
} 
 
// ===================================================== 
// SETUP 
// ===================================================== 
 
void setup() 
{ 
  Serial.begin(115200); 
 
  // =================================================== 
  // MOTOR PINS 
  // =================================================== 
 
  pinMode( 
    LEFT_MOTOR, 
    OUTPUT 
  ); 
 
  pinMode( 
    FRONT_MOTOR, 
    OUTPUT 
  ); 
 
  pinMode( 
    RIGHT_MOTOR, 
    OUTPUT 
  ); 
 
  pinMode( 
    BACK_MOTOR, 
    OUTPUT 
  ); 
 
  // Make sure all motors start OFF 
  allMotorsOff(); 
 
  // =================================================== 
  // WIFI STATION MODE 
  // =================================================== 
 
  WiFi.mode(WIFI_STA); 
 
  // =================================================== 
  // ESP-NOW CHANNEL 
  // =================================================== 
 
  esp_wifi_set_channel( 
    1, 
    WIFI_SECOND_CHAN_NONE 
  ); 
 
  // =================================================== 
  // PRINT MAC 
  // =================================================== 
 
  Serial.println(); 
  Serial.println( 
    "================================" 
  ); 
 
  Serial.println( 
    "       FOG-SAFE SMART JACKET" 
  ); 
 
  Serial.println( 
    "================================" 
  ); 
 
  Serial.print( 
    "Jacket MAC: " 
  ); 
 
  Serial.println( 
    WiFi.macAddress() 
  ); 
 
  Serial.println(); 
 
  // =================================================== 
  // ESP-NOW INIT 
  // =================================================== 
 
  if ( 
    esp_now_init() != ESP_OK 
  ) 
  { 
    Serial.println( 
      "ESP-NOW INIT FAILED" 
    ); 
 
    while (true) 
    { 
      delay(1000); 
    } 
  } 
 
  // =================================================== 
  // RECEIVE CALLBACK 
  // =================================================== 
 
  esp_now_register_recv_cb( 
    onDataRecv 
  ); 
 
  Serial.println( 
    "ESP-NOW READY" 
  ); 
 
  Serial.println(); 
 
  Serial.println( 
    "Motor mapping:" 
  ); 
 
  Serial.println( 
    "LEFT  -> GPIO25" 
  ); 
 
  Serial.println( 
    "FRONT -> GPIO26" 
  ); 
 
  Serial.println( 
    "RIGHT -> GPIO27" 
  ); 
 
  Serial.println( 
    "BACK  -> GPIO14" 
  ); 
 
  Serial.println(); 
 
  Serial.println( 
    "Waiting for vehicle..." 
  ); 
 
  Serial.println( 
    "================================" 
  ); 
} 
 
// ===================================================== 
// LOOP 
// ===================================================== 
 
void loop() 
{ 
  // ESP-NOW reception is handled 
  // by the callback. 
 
  delay(10); 
}...what are the pin connections?