#include <Arduino.h>

/*
 * NOTE FOR NANO CNC SHIELD V4:
 * The Nano CNC Shield V4 has a famous hardware design quirk where
 * the STEP and DIR pins are swapped compared to standard GRBL!
 * - D5 is actually STEP (connected to ESP32 GPIO 19)
 * - D2 is actually DIR  (connected to ESP32 GPIO 18)
 * - D8 is ENABLE        (connected to ESP32 GPIO 21)
 */

const int PIN_STEP   = 19;  // Nano Shield D5 (Real STEP pin)
const int PIN_DIR    = 18;  // Nano Shield D2 (Real DIR pin)
const int PIN_ENABLE = 21;  // Nano Shield D8 (ENABLE - Active LOW)

void stepMotor(long steps, bool clockwise, int pulseDelayMicros) {
  digitalWrite(PIN_DIR, clockwise ? HIGH : LOW);
  delayMicroseconds(10);

  for (long i = 0; i < steps; i++) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(pulseDelayMicros);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(pulseDelayMicros);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==============================================");
  Serial.println("  Testing Swapped STEP/DIR Pins on CNC Shield  ");
  Serial.println("==============================================");

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);

  // Enable driver (Active LOW)
  digitalWrite(PIN_ENABLE, LOW);

  Serial.println("Driver enabled. Starting in 1 second...");
  delay(1000);
}

void loop() {
  // Rotate Clockwise for 2000 steps at 800us delay (reliable, strong torque)
  Serial.println(">>> Rotating Clockwise (2000 steps)...");
  stepMotor(2000, true, 800);
  delay(500);

  // Rotate Counter-Clockwise for 2000 steps
  Serial.println(">>> Rotating Counter-Clockwise (2000 steps)...");
  stepMotor(2000, false, 800);
  delay(500);

  // Faster spin: 3000 steps at 500us delay
  Serial.println(">>> Faster Spin (3000 steps)...");
  stepMotor(3000, true, 500);
  delay(1000);
}
