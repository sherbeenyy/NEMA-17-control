# NEMA 17 Stepper Motor Control Guide
**ESP32 + Arduino Nano CNC Shield V4 + A4988 Driver**

---

## 1. System Architecture

```
+-----------------------------------------------------------------------------------+
| 1. THE BRAIN (ESP32)                                                              |
|    Runs C++ firmware. Outputs low-power 3.3V digital signals (a few mA).          |
+-----------------------------------------------------------------------------------+
                                         |
                                (5 Jumper Wires)
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| 2. THE ROUTER (Arduino Nano CNC Shield V4)                                        |
|    Routes signals from the Nano socket to the A4988 driver socket and combines    |
|    them with external 12V DC power from the barrel jack.                          |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| 3. THE DRIVER (A4988 Module)                                                      |
|    Listens to 3.3V logic pulses. Opens and closes internal MOSFET H-bridges to     |
|    send 12V high-current pulses into the motor coils.                             |
+-----------------------------------------------------------------------------------+
                                         |
                            (4-pin NEMA 17 cable)
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| 4. THE MOTOR (NEMA 17 Bipolar Stepper)                                            |
|    Two internal electromagnetic coils (Coil A & Coil B) rotate the magnetic rotor  |
|    by exactly 1.8 degrees per pulse.                                              |
+-----------------------------------------------------------------------------------+
```

---

## 2. Pin-by-Pin Wiring & Electrical Mapping

### Jumper Wires (ESP32 to Nano CNC Shield)

> [!WARNING]
> **Known Hardware Defect on Nano CNC Shield V4:**
> The silkscreen and traces on common red CNC Shield V4 boards swap Step and Direction!
> - **D5 is physically connected to STEP**
> - **D2 is physically connected to DIR**

| Wire # | ESP32 Pin | Shield Socket Pin | A4988 Pin | Signal Function | Electrical Behavior |
|:---:|:---|:---|:---|:---|:---|
| **1** | **GPIO 19** | **`D5`** | `STEP` | Step Pulse Generator | Each rising edge (0V $\rightarrow$ 3.3V) advances the rotor by 1 step ($1.8^\circ$). |
| **2** | **GPIO 18** | **`D2`** | `DIR` | Direction Selector | `HIGH` (3.3V) = Clockwise; `LOW` (0V) = Counter-Clockwise. |
| **3** | **GPIO 21** | **`D8`** | `ENABLE` | Driver Power Switch | **Active-LOW:** `LOW` = Coils energized (holding torque); `HIGH` = Driver disabled (coils unpowered, freewheels). |
| **4** | **3.3V** | **`5V`** | `VDD` | Logic Power Supply | Powers A4988 logic at 3.3V so that 3.3V ESP32 signals match its logic thresholds. |
| **5** | **GND** | **`GND`** | `GND` | Common Ground | Crucial reference: ties the ESP32 0V to the Shield 0V so signals don't float. |

---

## 3. Power & Motor Connections

```
        [ A4988 Driver ]
          |     |     |     |
         1B    1A    2A    2B
          |     |     |     |
     [ 4-Pin Header next to X Slot ]
          |     |     |     |
     [ Black Green  Red   Blue ] (Motor Cable)
          \___/       \___/
         Coil A       Coil B
```

1. **12V DC Barrel Jack:**
   * Center Pin = **+12V DC** (High current for motor coils).
   * Outer Ring = **12V Ground** (Power ground).
   * Plugging in the barrel jack powers the motor rail (`VMOT`).
2. **Motor 4-Pin Plug:**
   * Plugs into the 4-pin male header directly adjacent to the X-axis driver slot.
   * If the plug is flipped 180°, it simply reverses rotation direction without causing damage.

---

## 4. Safety Rules (Preventing Hardware Damage)

1. **Never unplug the 4-pin motor cable while 12V is powered:**
   Disconnecting an inductive coil under load creates a large inductive back-EMF spike that will destroy the A4988 instantly.
2. **A4988 Socket Orientation:**
   Always align the `EN` and `DIR` labels on the A4988 with the labels on the shield socket. Plugging it in backwards connects 12V to logic pins and destroys the module.
3. **Calibrating $V_{REF}$ (Current Limiting Potentiometer):**
   $$I_{max} \approx \frac{V_{REF}}{8 \times R_{sense}}$$
   For standard $R_{sense} = 0.1\Omega$ (marked `R100`), set $V_{REF}$ with a multimeter between the metal screw and GND:
   * Standard 1.0A NEMA 17 $\rightarrow$ **$V_{REF} \approx 0.6\text{V} - 0.7\text{V}$**.

---

## 5. Code Walkthrough (`src/main.cpp`)

### Variable Definitions
```cpp
const int PIN_STEP   = 19;  // Nano Shield D5 (Real STEP pin)
const int PIN_DIR    = 18;  // Nano Shield D2 (Real DIR pin)
const int PIN_ENABLE = 21;  // Nano Shield D8 (ENABLE - Active LOW)
```

### Setup Initialization
```cpp
void setup() {
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);

  // Pull ENABLE low to energize the motor coils
  digitalWrite(PIN_ENABLE, LOW);
}
```

### The Pulse Engine (`stepMotor`)
```cpp
void stepMotor(long steps, bool clockwise, int pulseDelayMicros) {
  // 1. Set Direction
  digitalWrite(PIN_DIR, clockwise ? HIGH : LOW);
  delayMicroseconds(10); // Setup time for driver logic

  // 2. Generate exact pulse train
  for (long i = 0; i < steps; i++) {
    digitalWrite(PIN_STEP, HIGH);          // Rising edge: Driver takes 1 step
    delayMicroseconds(pulseDelayMicros);   // Pulse high duration
    digitalWrite(PIN_STEP, LOW);           // Falling edge: Reset pulse
    delayMicroseconds(pulseDelayMicros);   // Pulse low duration (controls speed!)
  }
}
```

---

## 6. Precision & Angle Math

A standard NEMA 17 motor has **200 steps per revolution**:
$$360^\circ \div 200\text{ steps} = 1.8^\circ\text{ per step}$$

$$\text{Steps} = \text{Target Degrees} \times \left(\frac{200}{360}\right)$$

| Target Angle | Calculation | Steps Sent | Result |
|:---:|:---:|:---:|:---|
| **$1.8^\circ$** | $1.8 \times (200 / 360)$ | **1 step** | Single smallest physical step |
| **$45.0^\circ$** | $45.0 \times (200 / 360)$ | **25 steps** | $1/8$ of a circle |
| **$90.0^\circ$** | $90.0 \times (200 / 360)$ | **50 steps** | Quarter circle |
| **$180.0^\circ$** | $180.0 \times (200 / 360)$ | **100 steps** | Half circle |
| **$360.0^\circ$** | $360.0 \times (200 / 360)$ | **200 steps** | One complete revolution |

*(If microstepping jumpers are installed beneath the A4988 on the shield, 1 revolution = 3200 steps, meaning $1.8^\circ = 16\text{ steps}$ and 1 step = $0.1125^\circ$).*

---

## 7. Speed Math

Speed is set by `pulseDelayMicros`:

$$\text{Cycle Time per Step} = 2 \times \text{pulseDelayMicros}$$
$$\text{Steps per Second} = \frac{1,000,000}{2 \times \text{pulseDelayMicros}}$$
$$\text{RPM} = \frac{\text{Steps per Second}}{200} \times 60$$

* **`2000` $\mu$s delay:** 250 steps/sec $\rightarrow$ **75 RPM** (Slow, smooth, high precision)
* **`800` $\mu$s delay:** 625 steps/sec $\rightarrow$ **187.5 RPM** (Balanced speed and strong torque)
* **`400` $\mu$s delay:** 1,250 steps/sec $\rightarrow$ **375 RPM** (Fast continuous motion)

---

## 8. Development & Uploading with VS Code / PlatformIO

1. Open project in VS Code: `/home/ahmed/coding/dcmotor`.
2. Ensure **PlatformIO IDE** extension is installed.
3. Build code: Click **`✓`** or run:
   ```bash
   pio run
   ```
4. Upload to ESP32: Click **`→`** or run:
   ```bash
   pio run -t upload
   ```
5. Serial Monitor: Click **`🔌`** or run:
   ```bash
   pio device monitor
   ```
