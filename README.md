# ECE 306 Autonomous Car Project  

## Overview   
An embedded systems project featuring an autonomous vehicle built on the **MSP430FR2355** microcontroller. The car demonstrates advanced movement control, real-time sensor processing, wireless IoT communication, and intelligent line-following capabilities using infrared detection and adaptive state-machine navigation.

---

## Features  

### Core Capabilities
- **Autonomous Line Following**: IR-based black-line tracking with intelligent approach, align, and recovery behaviors
- **WiFi Remote Control**: IoT-enabled web interface for directional control via HTTP API
- **Real-Time PWM Motor Control**: Variable-speed forward/reverse/pivot with safety interlocks and trim calibration
- **State Machine Architecture**: Robust mode transitions (IDLE → SEARCH → APPROACH → ALIGN → TRACK → RECOVER)
- **Sensor Fusion**: Dual IR sensors with normalized intensity processing and adaptive thresholds
- **Serial Command Interface**: PC-based debugging and control via UART with authentication

### Advanced Behaviors
- **Stop-Align-Go Control Loop**: Precision line acquisition with motor-safe PWM enforcement (≥5000 counts)
- **Directional Recovery**: Departure-side detection for intelligent correction when leaving the track
- **Joystick Proportional Steering**: Web-based analog control with deadband and turn scaling
- **Calibration System**: White/black surface learning with dynamic threshold computation
- **LCD Real-Time Display**: 4-line status output showing sensor readings, states, and network info

---

## Hardware Components  

| Component | Description |
|-----------|-------------|
| **Microcontroller** | MSP430FR2355 (16-bit RISC, FRAM-based) |
| **IR Sensors** | Dual analog detectors (ADC channels 2 & 3) for line intensity |
| **Motors** | DC gear motors with H-bridge driver (PWM-controlled via TimerB3) |
| **LCD Display** | 4-line character display (HD44780 compatible) |
| **WiFi Module** | ESP32 IoT transceiver (UART serial @ 460800 baud) |
| **Power Supply** | Regulated 3.3V/5V rails with protection circuitry |
| **Switches** | SW1/SW2 with debounced interrupts for manual triggers |

---

## Architecture

### Software Modules

```
EmbeddedSystemCar/
├── main.c              # Core event loop and system initialization
├── IR.c/h              # Line-follow controller (560 lines, stop-align-go logic)
├── serial.c/h          # UART communication, IoT command parser, HTTP server
├── motors.c/h          # PWM generation, joystick control, trim calibration
├── wheels.c/h          # Movement queue with cooldown timing
├── ADC.c/h             # 12-bit sensor sampling (left/right/thumb channels)
├── timers.c/h          # TimerB0/B3 configuration (200ms tick, PWM)
├── interrupts.c        # ISR handlers (UART RX, ADC conversion, switches)
├── display.c/h         # LCD driver with BIG/normal font modes
├── ports.c/h           # GPIO initialization and pin mappings
├── stateMachine.c/h    # Boot sequence orchestration
└── switch.c/h          # Debounced button handlers
```

### State Machine Flow

```
IDLE ──[IRLine_BeginFollowing()]──> SEARCH
  │                                    │
  │                                    ├──[line detected]──> APPROACH
  │                                    └──[timeout]──────────┘
  │
APPROACH ──[both sensors locked]──> ALIGN
  │            └──[lost line]──> SEARCH
  │
ALIGN ──[centered & stable]──> TRACK
  │       └──[lost sensor]──> APPROACH
  │
TRACK ──[error too large]──> ALIGN
  │       └──[both sensors lost]──> RECOVER
  │
RECOVER ──[line reacquired]──> ALIGN
          └──[timeout]──> SEARCH
```

---

## How It Works

### Line-Following Algorithm

1. **Calibration Phase** (serial commands `W` / `Q`):
   - Sample white surface baseline → store `ADCLeft`, `ADCRight`
   - Sample black tape → compute thresholds with 100-count margin
   - Calculate normalized intensity span for each sensor

2. **Approach Logic**:
   - When either sensor detects the line, creep forward at 13k PWM with differential steering
   - Lost line → sweep decisively toward last-seen side (11.5k vs 5k PWM)
   - Lock confirmation: both sensors ≥640 intensity for 5 consecutive ticks → transition to ALIGN

3. **Align Phase**:
   - **Stop** motors immediately on entry
   - Compute error: `right_intensity - left_intensity`
   - Pivot single wheel at 12.5k PWM until error ≈ 0 and both sensors stable
   - Only proceed to TRACK when centered for 5 ticks

4. **Track Phase** (Go):
   - Drive at 14.5k base PWM with proportional correction (gain = 7200)
   - Error steering: `left_pwm = base + scaled_error`, `right_pwm = base - scaled_error`
   - If error exceeds ±520 counts → **stop and re-align**
   - If both sensors lose line → **stop and enter RECOVER**

5. **Recovery Logic**:
   - Record which sensor dropped first (left vs right)
   - Pause for 5 ticks, then pivot toward corrective side:
     - Left departed → steer right (7k left, 13.5k right)
     - Right departed → steer left (13.5k left, 7k right)
   - On reacquisition → return to ALIGN
   - Timeout (80 ticks) → return to SEARCH

### WiFi Remote Control

- **Web Interface**: `Webcontrol/joystick.html` provides virtual analog stick
- **HTTP API**: POST `/api/joystick` with `x` and `y` form data
- **Command Format**: Serial auth prefix `2005` + direction (`F`/`B`/`L`/`R`) + duration
  - Example: `2005F2000` → forward for 2000ms
  - `2005S` → immediate stop
- **IoT Commands**:
  - `I` → start line following
  - `D` → exit line mode (drive straight 3s)
  - `W`/`Q` → white/black calibration

### Motor Safety Features
- **PWM Floor Enforcement**: All non-zero commands ≥5000 to prevent stall/overheat
- **Direction Change Cooldown**: 0.4s pause when reversing to protect H-bridge
- **Conflict Detection**: Hardware interlock prevents simultaneous forward+reverse
- **Trim Calibration**: Per-wheel offsets compensate for motor asymmetry

---

## Getting Started

### Prerequisites
- **Code Composer Studio** 20.3.0 or later
- **MSP430FR2355 LaunchPad** development board
- **WiFi Network**: 2.4GHz with known SSID/password for ESP32 module
- **Serial Terminal**: PuTTY, Tera Term, or Termite

### Build & Flash

```bash
# Clone repository
git clone https://github.com/ohmptl/ECE306-Embedded-Systems-Car.git
cd ECE306-Embedded-Systems-Car/EmbeddedSystemCar

# Open in Code Composer Studio
File → Import → CCS Projects → Select search-directory → Browse to EmbeddedSystemCar/

# Build
Project → Build All (Ctrl+B)

# Flash to LaunchPad
Run → Debug (F11) → Resume (F8)
```

### WiFi Configuration

1. Connect PC serial terminal to USB (460800 baud, 8N1)
2. Press **SW1** to request WiFi status
3. Send AT command: `AT+CWJAP="your_ssid","your_password"`
4. Press **SW2** to request IP address
5. Open `Webcontrol/joystick.html` in browser, update IP in script
6. Use virtual joystick or send HTTP commands

### Line-Following Calibration

```
# Via serial terminal (115200 baud on UCA1):
2005W    # Calibrate white surface (place car on white)
2005Q    # Calibrate black tape (place car on tape)
2005I    # Start line following
2005D    # Stop and exit after 3 seconds
```

---

## Project Timeline

- **Sept 2025**: Initial motor control & state machine
- **Oct 2025**: ADC integration, IR sensor drivers
- **Nov 2025**: WiFi IoT server, HTTP command parser
- **Dec 2025**: Complete line-following rewrite with stop-align-go loop

---

## Technical Specifications

| Parameter | Value |
|-----------|-------|
| Clock Speed | 8 MHz (DCO) |
| PWM Frequency | ~50 kHz (TimerB3, 16-bit mode) |
| ADC Resolution | 10-bit (0–1023 counts) |
| Sensor Sample Rate | 5 Hz (200ms timer tick) |
| IR Intensity Range | 0–2048 normalized units |
| Base Track Speed | 14,500 PWM counts (~45% duty) |
| WiFi Baud Rate | 460800 (ESP8266 high-speed mode) |
| Command Latency | <200ms (network + processing) |

---

## Author

**Ohm Patel**  
ECE 306 Embedded Systems  
Fall 2025

---

## License

This project is developed for educational purposes as part of the ECE 306 curriculum at NCSU.  



