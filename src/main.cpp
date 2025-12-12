#include <Arduino.h>

// Laser control settings
#define LASER_PIN 21
#define PWM_FREQUENCY 16000  // 16kHz
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8     // 8-bit resolution (0-255)

// Safety timeout settings
#define TIMEOUT_MS 30000     // 30 seconds timeout for power >50%
#define HIGH_POWER_THRESHOLD 50  // Power level that triggers timeout

uint8_t laserPower = 0;  // Current laser power (0-100)
unsigned long lastActivityTime = 0;
bool timeoutActive = false;

void setLaserPower(uint8_t power) {
  // SAFETY: Invalid values default to OFF
  if (power > 100) {
    power = 0;
    Serial.println("WARNING: Invalid power value! Laser set to 0% for safety.");
  }
  
  laserPower = power;
  
  // Convert 0-100 to 0-255 PWM duty cycle
  uint8_t dutyCycle = map(power, 0, 100, 0, 255);
  
  // Set PWM duty cycle
  ledcWrite(PWM_CHANNEL, dutyCycle);
  
  Serial.print("Laser power set to: ");
  Serial.print(power);
  Serial.print("% (PWM: ");
  Serial.print(dutyCycle);
  Serial.println("/255)");
  
  // SAFETY: Reset timeout timer when power changes
  lastActivityTime = millis();
  timeoutActive = (power > HIGH_POWER_THRESHOLD);
  
  if (timeoutActive) {
    Serial.print("SAFETY: Timeout active - laser will auto-disable in ");
    Serial.print(TIMEOUT_MS / 1000);
    Serial.println(" seconds");
  }
}

void checkSafetyTimeout() {
  // Only check timeout if laser is at high power
  if (!timeoutActive) return;
  
  unsigned long currentTime = millis();
  
  // Check for timeout (accounting for millis() rollover)
  if (currentTime - lastActivityTime >= TIMEOUT_MS) {
    Serial.println("===================================");
    Serial.println("SAFETY TIMEOUT: Auto-disabling laser!");
    Serial.println("Laser was at high power (>50%) for too long");
    Serial.println("===================================");
    setLaserPower(0);
  }
}

void setup() {
  // SAFETY: Configure PWM BEFORE attaching pin to ensure it starts at 0
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcWrite(PWM_CHANNEL, 0);  // Explicitly set to 0 before pin attachment
  ledcAttachPin(LASER_PIN, PWM_CHANNEL);
  
  // Initialize serial
  Serial.begin(115200);
  Serial.println("===================================");
  Serial.println("CLASS 4 LASER PWM Controller");
  Serial.println("===================================");
  Serial.println("SAFETY: Laser starts at 0% power");
  Serial.println("Send power value 0-100 via serial");
  Serial.println("Invalid values default to 0% (OFF)");
  Serial.print("SAFETY: Auto-timeout at >50% power: ");
  Serial.print(TIMEOUT_MS / 1000);
  Serial.println(" seconds");
  Serial.println("===================================");
  
  // SAFETY: Explicitly confirm laser is off
  setLaserPower(0);
  lastActivityTime = millis();
}

void loop() {
  // SAFETY: Check for timeout condition
  checkSafetyTimeout();
  
  // Check for serial input
  if (Serial.available()) {
    // Read the incoming value
    int value = Serial.parseInt();
    
    // Consume any remaining characters (like newline)
    while (Serial.available()) {
      Serial.read();
    }
    
    // SAFETY: Only accept valid range, otherwise set to 0
    if (value >= 0 && value <= 100) {
      setLaserPower(value);
    } else {
      Serial.print("ERROR: Invalid value (");
      Serial.print(value);
      Serial.println("). Setting laser to 0% for safety.");
      setLaserPower(0);
    }
  }
}