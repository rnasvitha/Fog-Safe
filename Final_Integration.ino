#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050.h>

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// =====================================================
// OLED
// =====================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
// FRONT ULTRASONIC
// =====================================================
#define TRIG_FRONT 5
#define ECHO_FRONT 34

// =====================================================
// RIGHT ULTRASONIC
// =====================================================
#define TRIG_RIGHT 18
#define ECHO_RIGHT 35

// =====================================================
// LEFT ULTRASONIC
// =====================================================
#define TRIG_LEFT 19
#define ECHO_LEFT 32

// =====================================================
// IR SENSOR
// =====================================================
#define IR_PIN 26

// =====================================================
// BUZZER
// =====================================================
#define BUZZER_PIN 4

// =====================================================
// LDR / FOG SENSOR
// GPIO36 = VP
// INPUT ONLY - perfect for analog LDR
// =====================================================
#define LDR_PIN 36

// =====================================================
// FOG THRESHOLD
// Clear ≈ 2100
// Mist  ≈ 3100
// =====================================================
#define FOG_THRESHOLD 2600

// =====================================================
// DISTANCE THRESHOLDS
// =====================================================
#define CRITICAL_DISTANCE 10
#define WARNING_DISTANCE 30

// =====================================================
// MPU6050
// =====================================================
MPU6050 mpu;

// =====================================================
// SENSOR VARIABLES
// =====================================================
long frontDistance = 999;
long rightDistance = 999;
long leftDistance = 999;

int ldrValue = 0;

bool fogDetected = false;
bool irDetected = false;

// =====================================================
// V2V SIMULATION
// =====================================================
bool simulatedV2V = false;

// =====================================================
// ESP-NOW
// =====================================================
uint8_t broadcastAddress[] = {
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  0xFF
};

typedef struct struct_message
{
  char command[20];
} struct_message;

struct_message message;

// =====================================================
// BUZZER TIMING
// =====================================================
unsigned long lastBuzzerTime = 0;
bool buzzerState = false;

// =====================================================
// READ ULTRASONIC
// =====================================================
long readUltrasonic(
  int trigPin,
  int echoPin
)
{
  digitalWrite(
    trigPin,
    LOW
  );

  delayMicroseconds(2);

  digitalWrite(
    trigPin,
    HIGH
  );

  delayMicroseconds(10);

  digitalWrite(
    trigPin,
    LOW
  );

  long duration = pulseIn(
    echoPin,
    HIGH,
    30000
  );

  if (duration == 0)
  {
    return 999;
  }

  long distance =
    duration * 0.034 / 2;

  return distance;
}

// =====================================================
// READ LDR / FOG
// =====================================================
void readLDR()
{
  ldrValue = analogRead(LDR_PIN);

  if (ldrValue > FOG_THRESHOLD)
  {
    fogDetected = true;
  }
  else
  {
    fogDetected = false;
  }
}

// =====================================================
// SEND ESP-NOW COMMAND
// =====================================================
void sendCommand(
  const char *cmd
)
{
  memset(
    &message,
    0,
    sizeof(message)
  );

  strncpy(
    message.command,
    cmd,
    sizeof(message.command) - 1
  );

  esp_now_send(
    broadcastAddress,
    (uint8_t *)&message,
    sizeof(message)
  );
}

// =====================================================
// BUZZER CONTROL
// =====================================================
void handleBuzzer(
  bool critical
)
{
  // -----------------------------------------------
  // NO CRITICAL CONDITION
  // -----------------------------------------------
  if (!critical)
  {
    digitalWrite(
      BUZZER_PIN,
      LOW
    );

    buzzerState = false;

    return;
  }

  // -----------------------------------------------
  // CRITICAL CONDITION
  // -----------------------------------------------
  unsigned long now =
    millis();

  // OFF -> ON
  if (!buzzerState)
  {
    if (
      now - lastBuzzerTime >= 120
    )
    {
      buzzerState = true;

      digitalWrite(
        BUZZER_PIN,
        HIGH
      );

      lastBuzzerTime = now;
    }
  }

  // ON -> OFF
  else
  {
    if (
      now - lastBuzzerTime >= 180
    )
    {
      buzzerState = false;

      digitalWrite(
        BUZZER_PIN,
        LOW
      );

      lastBuzzerTime = now;
    }
  }
}

// =====================================================
// OLED
// =====================================================
void updateOLED(
  bool critical,
  bool warning
)
{
  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  // =================================================
  // SENSOR VALUES
  // =================================================
  display.setTextSize(1);

  display.setCursor(0, 0);

  display.print("F:");
  display.print(frontDistance);

  display.print(" R:");
  display.print(rightDistance);

  // -----------------------------------------------
  // LEFT + IR
  // -----------------------------------------------
  display.setCursor(0, 10);

  display.print("L:");
  display.print(leftDistance);

  display.print(" IR:");
  display.print(
    irDetected ? "YES" : "NO"
  );

  // =================================================
  // FOG
  // =================================================
  display.setCursor(0, 20);

  if (fogDetected)
  {
    display.print(
      "FOG: DETECTED"
    );
  }
  else
  {
    display.print(
      "FOG: CLEAR"
    );
  }

  // =================================================
  // DIVIDER
  // =================================================
  display.drawLine(
    0,
    29,
    127,
    29,
    SSD1306_WHITE
  );

  // =================================================
  // MAIN STATUS
  // =================================================
  display.setTextSize(2);

  display.setCursor(0, 32);

  if (critical)
  {
    display.println(
      "CRITICAL"
    );
  }
  else if (warning)
  {
    display.println(
      "WARNING"
    );
  }
  else
  {
    display.println(
      "SAFE"
    );
  }

  // =================================================
  // LDR VALUE
  // =================================================
  display.setTextSize(1);

  display.setCursor(0, 55);

  display.print("LDR:");
  display.print(ldrValue);

  display.display();
}

// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(115200);

  // =================================================
  // ULTRASONIC PINS
  // =================================================
  pinMode(
    TRIG_FRONT,
    OUTPUT
  );

  pinMode(
    ECHO_FRONT,
    INPUT
  );

  pinMode(
    TRIG_RIGHT,
    OUTPUT
  );

  pinMode(
    ECHO_RIGHT,
    INPUT
  );

  pinMode(
    TRIG_LEFT,
    OUTPUT
  );

  pinMode(
    ECHO_LEFT,
    INPUT
  );

  // =================================================
  // IR
  // =================================================
  pinMode(
    IR_PIN,
    INPUT
  );

  // =================================================
  // BUZZER
  // =================================================
  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  // =================================================
  // LDR
  // GPIO36 is INPUT ONLY
  // =================================================
  pinMode(
    LDR_PIN,
    INPUT
  );

  // =================================================
  // I2C
  // =================================================
  Wire.begin(
    21,
    22
  );

  // =================================================
  // OLED
  // =================================================
  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDR
    )
  )
  {
    Serial.println(
      "OLED FAILED"
    );

    while (1);
  }

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(
    "FOG-SAFE SYSTEM"
  );

  display.setCursor(0, 12);
  display.println(
    "Starting..."
  );

  display.display();

  delay(1500);

  // =================================================
  // MPU6050
  // =================================================
  mpu.initialize();

  if (
    mpu.testConnection()
  )
  {
    Serial.println(
      "MPU6050 OK"
    );
  }
  else
  {
    Serial.println(
      "MPU6050 FAILED"
    );
  }

  // =================================================
  // WIFI
  // =================================================
  WiFi.mode(
    WIFI_STA
  );

  // ESP-NOW channel
  esp_wifi_set_channel(
    1,
    WIFI_SECOND_CHAN_NONE
  );

  Serial.print(
    "ESP32 MAC: "
  );

  Serial.println(
    WiFi.macAddress()
  );

  // =================================================
  // ESP-NOW INIT
  // =================================================
  if (
    esp_now_init() != ESP_OK
  )
  {
    Serial.println(
      "ESP-NOW INIT FAILED"
    );

    return;
  }

  // =================================================
  // ADD BROADCAST PEER
  // =================================================
  esp_now_peer_info_t peerInfo = {};

  memcpy(
    peerInfo.peer_addr,
    broadcastAddress,
    6
  );

  peerInfo.channel = 1;
  peerInfo.encrypt = false;

  if (
    esp_now_add_peer(
      &peerInfo
    ) != ESP_OK
  )
  {
    Serial.println(
      "ESP-NOW PEER FAILED"
    );
  }
  else
  {
    Serial.println(
      "ESP-NOW READY"
    );
  }

  // =================================================
  // STARTUP INFO
  // =================================================
  Serial.println();
  Serial.println(
    "======================================"
  );

  Serial.println(
    "          FOG-SAFE MAIN VEHICLE"
  );

  Serial.println(
    "======================================"
  );

  Serial.println();

  Serial.println(
    "FRONT ULTRASONIC : TRIG 5 / ECHO 34"
  );

  Serial.println(
    "RIGHT ULTRASONIC : TRIG 18 / ECHO 35"
  );

  Serial.println(
    "LEFT ULTRASONIC  : TRIG 19 / ECHO 32"
  );

  Serial.println(
    "IR SENSOR        : GPIO 26"
  );

  Serial.println(
    "BUZZER           : GPIO 4"
  );

  Serial.println(
    "LDR              : GPIO 36 / VP"
  );

  Serial.println(
    "MPU6050 SDA      : GPIO 21"
  );

  Serial.println(
    "MPU6050 SCL      : GPIO 22"
  );

  Serial.println(
    "OLED             : 0x3C"
  );

  Serial.println();

  Serial.print(
    "Critical distance : "
  );

  Serial.print(
    CRITICAL_DISTANCE
  );

  Serial.println(
    " cm"
  );

  Serial.print(
    "Warning distance  : "
  );

  Serial.print(
    WARNING_DISTANCE
  );

  Serial.println(
    " cm"
  );

  Serial.print(
    "Fog threshold     : "
  );

  Serial.println(
    FOG_THRESHOLD
  );

  Serial.println();

  Serial.println(
    "V2V TEST:"
  );

  Serial.println(
    "Send 1 = CRITICAL"
  );

  Serial.println(
    "Send 0 = CLEAR"
  );

  Serial.println();

  Serial.println(
    "======================================"
  );
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop()
{
  // =================================================
  // FRONT DISTANCE
  // =================================================
  frontDistance =
    readUltrasonic(
      TRIG_FRONT,
      ECHO_FRONT
    );

  delay(5);

  // =================================================
  // RIGHT DISTANCE
  // =================================================
  rightDistance =
    readUltrasonic(
      TRIG_RIGHT,
      ECHO_RIGHT
    );

  delay(5);

  // =================================================
  // LEFT DISTANCE
  // =================================================
  leftDistance =
    readUltrasonic(
      TRIG_LEFT,
      ECHO_LEFT
    );

  // =================================================
  // IR
  // Most obstacle IR modules are LOW when detected
  // =================================================
  irDetected =
    (digitalRead(IR_PIN) == LOW);

  // =================================================
  // LDR
  // =================================================
  readLDR();

  // =================================================
  // RISK FLAGS
  // =================================================
  bool critical = false;
  bool warning = false;

  // =================================================
  // CRITICAL DISTANCE
  // =================================================
  if (
    frontDistance <
    CRITICAL_DISTANCE
  )
  {
    critical = true;
  }

  if (
    rightDistance <
    CRITICAL_DISTANCE
  )
  {
    critical = true;
  }

  if (
    leftDistance <
    CRITICAL_DISTANCE
  )
  {
    critical = true;
  }

  // =================================================
  // IR CRITICAL
  // =================================================
  if (irDetected)
  {
    critical = true;
  }

  // =================================================
  // V2V CRITICAL
  // =================================================
  if (simulatedV2V)
  {
    critical = true;
  }

  // =================================================
  // WARNING
  // =================================================
  if (!critical)
  {
    if (
      frontDistance <
      WARNING_DISTANCE
    )
    {
      warning = true;
    }

    if (
      rightDistance <
      WARNING_DISTANCE
    )
    {
      warning = true;
    }

    if (
      leftDistance <
      WARNING_DISTANCE
    )
    {
      warning = true;
    }
  }

  // =================================================
  // SEND JACKET COMMAND
  // =================================================

  if (critical)
  {
    sendCommand(
      "CRITICAL"
    );
  }

  else if (
    frontDistance <
    WARNING_DISTANCE
  )
  {
    sendCommand(
      "FRONT"
    );
  }

  else if (
    leftDistance <
    WARNING_DISTANCE
  )
  {
    sendCommand(
      "LEFT"
    );
  }

  else if (
    rightDistance <
    WARNING_DISTANCE
  )
  {
    sendCommand(
      "RIGHT"
    );
  }

  else
  {
    sendCommand(
      "STOP"
    );
  }

  // =================================================
  // BUZZER
  // =================================================
  handleBuzzer(
    critical
  );

  // =================================================
  // OLED
  // =================================================
  updateOLED(
    critical,
    warning
  );

  // =================================================
  // SERIAL TELEMETRY
  // =================================================

  Serial.print(
    "FRONT="
  );

  Serial.print(
    frontDistance
  );

  Serial.print(
    "cm | RIGHT="
  );

  Serial.print(
    rightDistance
  );

  Serial.print(
    "cm | LEFT="
  );

  Serial.print(
    leftDistance
  );

  Serial.print(
    "cm | IR="
  );

  Serial.print(
    irDetected
      ? "DETECTED"
      : "CLEAR"
  );

  Serial.print(
    " | LDR="
  );

  Serial.print(
    ldrValue
  );

  Serial.print(
    " | FOG="
  );

  Serial.print(
    fogDetected
      ? "DETECTED"
      : "CLEAR"
  );

  Serial.print(
    " | V2V="
  );

  Serial.print(
    simulatedV2V
      ? "CRITICAL"
      : "CLEAR"
  );

  Serial.print(
    " | STATUS="
  );

  if (critical)
  {
    Serial.println(
      "CRITICAL"
    );
  }

  else if (warning)
  {
    Serial.println(
      "WARNING"
    );
  }

  else
  {
    Serial.println(
      "SAFE"
    );
  }

  // =================================================
  // V2V SERIAL TEST
  // =================================================

  if (Serial.available())
  {
    char c =
      Serial.read();

    // -----------------------------------------------
    // 1 = V2V CRITICAL
    // -----------------------------------------------
    if (c == '1')
    {
      simulatedV2V = true;

      Serial.println(
        "V2V TEST = CRITICAL"
      );
    }

    // -----------------------------------------------
    // 0 = V2V CLEAR
    // -----------------------------------------------
    if (c == '0')
    {
      simulatedV2V = false;

      Serial.println(
        "V2V TEST = CLEAR"
      );
    }
  }

  delay(100);
}