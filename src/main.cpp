#include <Arduino.h>

/*
 * NOTE FOR NANO CNC SHIELD V4:
 * The Nano CNC Shield V4 has a famous hardware design quirk where
 * the STEP and DIR pins are swapped compared to standard GRBL!
 * X: D5 is actually STEP (ESP32 GPIO 19), D2 is actually DIR (ESP32 GPIO 18)
 * Y: D6 is actually STEP (ESP32 GPIO 25), D3 is actually DIR (ESP32 GPIO 26)
 * Z: D7 is actually STEP (ESP32 GPIO 32), D4 is actually DIR (ESP32 GPIO 33)
 * D8 is ENABLE for all drivers (ESP32 GPIO 21)
 */

const int X_STEP = 19;  // Nano Shield D5 (Real STEP pin)
const int X_DIR  = 18;  // Nano Shield D2 (Real DIR pin)

const int Y_STEP = 25;  // Nano Shield D6 (Real STEP pin)
const int Y_DIR  = 26;  // Nano Shield D3 (Real DIR pin)

const int Z_STEP = 32;  // Nano Shield D7 (Real STEP pin)
const int Z_DIR  = 33;  // Nano Shield D4 (Real DIR pin)

const int PIN_ENABLE = 21;  // Nano Shield D8 (ENABLE - Active LOW, shared)

void stepMotor(int stepPin, int dirPin, long steps, bool clockwise, int pulseDelayMicros) {
  digitalWrite(dirPin, clockwise ? HIGH : LOW);
  delayMicroseconds(10);

  for (long i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseDelayMicros);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(pulseDelayMicros);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==============================================");
  Serial.println("  Testing Swapped STEP/DIR Pins on CNC Shield  ");
  Serial.println("==============================================");

  pinMode(X_STEP, OUTPUT);
  pinMode(X_DIR, OUTPUT);
  pinMode(Y_STEP, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
  pinMode(Z_STEP, OUTPUT);
  pinMode(Z_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);

  // Enable drivers (Active LOW)
  digitalWrite(PIN_ENABLE, LOW);

  Serial.println("Drivers enabled. Starting in 1 second...");
  delay(1000);
}

void loop() {
  // ===== X MOTOR =====
  // Rotate Clockwise for 2000 steps at 800us delay (reliable, strong torque)
  Serial.println(">>> X: Rotating Clockwise (2000 steps)...");
  stepMotor(X_STEP, X_DIR, 2000, true, 800);
  delay(500);

  // Rotate Counter-Clockwise for 2000 steps
  Serial.println(">>> X: Rotating Counter-Clockwise (2000 steps)...");
  stepMotor(X_STEP, X_DIR, 2000, false, 800);
  delay(500);

  // Faster spin: 3000 steps at 500us delay
  Serial.println(">>> X: Faster Spin (3000 steps)...");
  stepMotor(X_STEP, X_DIR, 3000, true, 500);
  delay(1000);

  // ===== Y MOTOR =====
  Serial.println(">>> Y: Rotating Clockwise (2000 steps)...");
  stepMotor(Y_STEP, Y_DIR, 2000, true, 800);
  delay(500);

  Serial.println(">>> Y: Rotating Counter-Clockwise (2000 steps)...");
  stepMotor(Y_STEP, Y_DIR, 2000, false, 800);
  delay(500);

  Serial.println(">>> Y: Faster Spin (3000 steps)...");
  stepMotor(Y_STEP, Y_DIR, 3000, true, 500);
  delay(1000);

  // ===== Z MOTOR =====
  Serial.println(">>> Z: Rotating Clockwise (2000 steps)...");
  stepMotor(Z_STEP, Z_DIR, 2000, true, 800);
  delay(500);

  Serial.println(">>> Z: Rotating Counter-Clockwise (2000 steps)...");
  stepMotor(Z_STEP, Z_DIR, 2000, false, 800);
  delay(500);

  Serial.println(">>> Z: Faster Spin (3000 steps)...");
  stepMotor(Z_STEP, Z_DIR, 3000, true, 500);
  delay(1000);
}
