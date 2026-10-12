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
|    by exact discrete step angles.                                                 |
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
| **1** | **GPIO 19** | **`D5`** | `STEP` | Step Pulse Generator | Each rising edge (0V $\rightarrow$ 3.3V) advances the rotor by 1 step. |
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

## 4. Complete Mathematical Calculations & Formulas

### A. Angular Precision & Step Calculations

A standard NEMA 17 stepper motor has an inherent physical step angle:
$$\theta_{\text{step}} = 1.8^\circ$$

The base steps per revolution in **Full-Step mode** is:
$$N_{\text{spr}} = \frac{360^\circ}{1.8^\circ} = 200\text{ steps/revolution}$$

#### Microstepping Resolution ($M$):
The A4988 can subdivide each physical step into microsteps using the MS1, MS2, MS3 pins:
$$S_{\text{rev}} = 200 \times M$$
$$\text{Angular Resolution per Step } (\theta_{\text{res}}) = \frac{1.8^\circ}{M} = \frac{360^\circ}{S_{\text{rev}}}$$

| Microstep Mode ($M$) | MS1, MS2, MS3 Jumpers | Steps / Revolution ($S_{\text{rev}}$) | Step Angle ($\theta_{\text{res}}$) | Precision Description |
|:---|:---:|:---:|:---:|:---|
| **Full Step ($1\times$)** | All Open (LOW) | **200** | **$1.8000^\circ$** | Highest torque, audible stepping |
| **Half Step ($2\times$)** | MS1 ON | **400** | **$0.9000^\circ$** | Smoother |
| **Quarter Step ($4\times$)** | MS2 ON | **800** | **$0.4500^\circ$** | Great general precision |
| **Eighth Step ($8\times$)** | MS1 + MS2 ON | **1,600** | **$0.2250^\circ$** | Very quiet and smooth |
| **Sixteenth Step ($16\times$)**| MS1 + MS2 + MS3 ON | **3,200** | **$0.1125^\circ$** | Ultra-fine angular resolution |

#### Formula: Target Angle to Number of Steps
To rotate by any desired angle $\theta$ (in degrees):
$$\text{Steps} = \text{round}\left(|\theta| \times \frac{S_{\text{rev}}}{360^\circ}\right)$$

* **In Full-Step Mode ($S_{\text{rev}} = 200$):**
  $$\text{Steps} = |\theta| \times \frac{200}{360} = \frac{|\theta|}{1.8}$$
* **In 1/16 Microstepping Mode ($S_{\text{rev}} = 3200$):**
  $$\text{Steps} = |\theta| \times \frac{3200}{360} = |\theta| \times 8.8889$$

#### Formula: Number of Steps to Resulting Angle
$$\theta = \text{Steps} \times \frac{360^\circ}{S_{\text{rev}}}$$

---

### B. Speed, Frequency, Delay & RPM Calculations

Each step is commanded by toggling `PIN_STEP` HIGH, waiting `pulseDelayMicros` ($\mu\text{s}$), toggling LOW, and waiting `pulseDelayMicros`:

$$\text{Period of 1 Step } (T_{\text{step}}) = 2 \times \text{pulseDelayMicros } [\mu\text{s}]$$

#### Step Frequency ($f_{\text{step}}$ in Steps per Second / Hz):
$$f_{\text{step}} = \frac{1,000,000}{T_{\text{step}}} = \frac{1,000,000}{2 \times \text{pulseDelayMicros}}$$

#### Revolutions Per Minute (RPM):
$$\text{RPM} = \frac{f_{\text{step}}}{S_{\text{rev}}} \times 60 = \frac{60,000,000}{2 \times \text{pulseDelayMicros} \times S_{\text{rev}}} = \frac{30,000,000}{\text{pulseDelayMicros} \times S_{\text{rev}}}$$

#### Inverse Formula: Desired RPM to `pulseDelayMicros`:
$$\text{pulseDelayMicros} = \frac{30,000,000}{\text{RPM} \times S_{\text{rev}}}$$

#### Speed Calculation Reference Table (Full-Step Mode, $S_{\text{rev}} = 200$):
| `pulseDelayMicros` | Cycle Time ($T_{\text{step}}$) | Pulses/Sec ($f_{\text{step}}$) | Resulting RPM | Motion Characteristics |
|:---:|:---:|:---:|:---:|:---|
| **$5000\mu\text{s}$ (5.0 ms)** | $10,000\mu\text{s}$ | $100\text{ steps/s}$ | **30 RPM** | Slow, maximum torque |
| **$2500\mu\text{s}$ (2.5 ms)** | $5,000\mu\text{s}$ | $200\text{ steps/s}$ | **60 RPM** (1 rev/s) | Steady positioning |
| **$1250\mu\text{s}$ (1.25 ms)**| $2,500\mu\text{s}$ | $400\text{ steps/s}$ | **120 RPM** | Smooth standard speed |
| **$800\mu\text{s}$ (0.8 ms)** | $1,600\mu\text{s}$ | $625\text{ steps/s}$ | **187.5 RPM** | Strong working speed |
| **$500\mu\text{s}$ (0.5 ms)** | $1,000\mu\text{s}$ | $1,000\text{ steps/s}$ | **300 RPM** | Fast motion |
| **$350\mu\text{s}$ (0.35 ms)**| $700\mu\text{s}$ | $1,428\text{ steps/s}$ | **428.5 RPM** | Rapid limit (requires ramp) |

---

### C. Linear Motion Calculations (For CNC / 3D Printer Axes)

When connecting the motor to a mechanical transmission to move an axis linearly:

#### 1. Timing Belt & Pulley System (e.g. GT2 Belt on X/Y Axis):
* GT2 Belt Pitch ($p$): $2.0\text{ mm}$
* Pulley Tooth Count ($Z$): typically 16 or 20 teeth
* Travel per revolution ($L_{\text{rev}}$):
  $$L_{\text{rev}} = Z \times p$$
  *(For 20-tooth pulley: $20 \times 2\text{ mm} = 40.0\text{ mm/rev}$)*
  *(For 16-tooth pulley: $16 \times 2\text{ mm} = 32.0\text{ mm/rev}$)*

$$\text{Steps per mm} = \frac{S_{\text{rev}}}{L_{\text{rev}}} = \frac{S_{\text{rev}}}{Z \times p}$$

* **Example (20T pulley, 1/16 Microstepping $S_{\text{rev}} = 3200$):**
  $$\text{Steps per mm} = \frac{3200}{40\text{ mm}} = 80.0\text{ steps/mm}$$
  $$\text{Linear Resolution} = \frac{1}{80} = 0.0125\text{ mm} = 12.5\mu\text{m}$$

#### 2. Lead Screw System (e.g. T8 Lead Screw on Z-Axis):
* Lead ($L$): Distance traveled per single revolution (standard T8-8 is $8.0\text{ mm/rev}$).

$$\text{Steps per mm} = \frac{S_{\text{rev}}}{\text{Lead}}$$

* **Example (T8-8 Lead Screw, 1/16 Microstepping $S_{\text{rev}} = 3200$):**
  $$\text{Steps per mm} = \frac{3200}{8\text{ mm}} = 400.0\text{ steps/mm}$$
  $$\text{Linear Resolution} = \frac{1}{400} = 0.0025\text{ mm} = 2.5\mu\text{m}$$

#### Formula: Target Linear Distance to Steps
$$\text{Steps} = \text{Distance (in mm)} \times \text{Steps per mm}$$

---

### D. Current Limiting ($V_{\text{REF}}$) & Electrical Calculations

The A4988 uses internal current sense resistors ($R_{\text{sense}}$) and reference voltage ($V_{\text{REF}}$) to limit current through the motor coils:

$$I_{\text{tripMAX}} = \frac{V_{\text{REF}}}{8 \times R_{\text{sense}}}$$

$$V_{\text{REF}} = I_{\text{tripMAX}} \times 8 \times R_{\text{sense}}$$

#### Identifying $R_{\text{sense}}$ on your A4988 Board:
Inspect the two tiny SMD resistors near the motor pins on the A4988 board:
* **Marked `R100`:** $R_{\text{sense}} = 0.10\Omega \implies V_{\text{REF}} = I_{\text{max}} \times 0.8$
* **Marked `R050`:** $R_{\text{sense}} = 0.05\Omega \implies V_{\text{REF}} = I_{\text{max}} \times 0.4$
* **Marked `R200`:** $R_{\text{sense}} = 0.20\Omega \implies V_{\text{REF}} = I_{\text{max}} \times 1.6$

#### The 70% Operating Rule (Thermal Derating):
Running stepper motors at 100% rated current causes them to reach $80^\circ\text{C}-100^\circ\text{C}$ and can overheat the A4988. Set target current to **70%–80%** of motor rated current:
$$I_{\text{target}} = 0.70 \times I_{\text{rated}}$$

| Motor Rated Current ($I_{\text{rated}}$) | Target Safe Current ($I_{\text{target}}$) | Target $V_{\text{REF}}$ (with `R100` = $0.1\Omega$) |
|:---:|:---:|:---:|
| **0.8 A** | $0.56\text{ A}$ | **$0.45\text{ V}$** |
| **1.0 A** | $0.70\text{ A}$ | **$0.56\text{ V}$** |
| **1.2 A** | $0.84\text{ A}$ | **$0.67\text{ V}$** |
| **1.5 A** | $1.05\text{ A}$ | **$0.84\text{ V}$** |
| **1.7 A** | $1.20\text{ A}$ | **$0.96\text{ V}$** *(Requires heatsink + fan)* |

---

### E. Power Supply Sizing for Multiple Motors

When adding more motors (e.g. 3 motors on X, Y, Z axes of the CNC shield):

$$I_{\text{supply}} \ge \left( \sum_{i=1}^{N} I_{\text{motor}, i} \right) \times 1.25\text{ (headroom factor)}$$

* **For 1 Motor @ 1.0A:** $1.0\text{A} \times 1.25 = \mathbf{1.25\text{A}}$ minimum (12V 2A adapter is ideal).
* **For 3 Motors @ 1.0A:** $3.0\text{A} \times 1.25 = \mathbf{3.75\text{A}}$ minimum (12V 4A or 5A power brick).
* **Total Power ($P$ in Watts):**
  $$P = V_{\text{supply}} \times I_{\text{supply}} = 12\text{V} \times 4\text{A} = 48\text{ Watts}$$

---

## 5. Software Implementation (`src/main.cpp`)

```cpp
#include <Arduino.h>

const int PIN_STEP   = 19;  // Nano Shield D5 (Real STEP pin on V4)
const int PIN_DIR    = 18;  // Nano Shield D2 (Real DIR pin on V4)
const int PIN_ENABLE = 21;  // Nano Shield D8 (ENABLE - Active LOW)

const int STEPS_PER_REV = 200; // Change to 3200 if using 1/16 microstepping

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

/**
 * Rotate by an exact angle in degrees at a specific RPM.
 */
void rotateAngleRPM(float degrees, float targetRPM) {
  bool clockwise = (degrees >= 0);
  float absDeg = abs(degrees);

  // 1. Calculate steps from angle
  long steps = round((absDeg / 360.0) * STEPS_PER_REV);

  // 2. Calculate pulseDelayMicros from target RPM
  // pulseDelayMicros = 30,000,000 / (RPM * STEPS_PER_REV)
  int pulseDelayMicros = round(30000000.0 / (targetRPM * STEPS_PER_REV));

  // Safety clamp on minimum delay
  if (pulseDelayMicros < 350) pulseDelayMicros = 350;

  stepMotor(steps, clockwise, pulseDelayMicros);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);

  // Enable driver
  digitalWrite(PIN_ENABLE, LOW);
}

void loop() {
  // Move 90 degrees at 60 RPM
  rotateAngleRPM(90.0, 60.0);
  delay(1000);

  // Move 360 degrees backwards at 120 RPM
  rotateAngleRPM(-360.0, 120.0);
  delay(2000);
}
```

---

## 6. Development & Upload Commands (PlatformIO / VS Code)

* **Compile Firmware:**
  ```bash
  pio run
  ```
* **Upload Firmware to ESP32:**
  ```bash
  pio run -t upload
  ```
* **Open Serial Debug Monitor (115200 baud):**
  ```bash
  pio device monitor
  ```
