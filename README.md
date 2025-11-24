# ECE 306 Autonomous Car Project  

## Overview  
This project was developed as part of **ECE 306: Introduction to Embedded Systems** at NC State University.  
The goal is to design and implement an **autonomous car** using the MSP430 microcontroller. The car demonstrates movement control, state-machine-based navigation, and sensor-driven behavior.  

## Features  
- **Finite State Machine (FSM)** for controlling movement sequences  
- Pre-programmed driving patterns (straight, circle, triangle, figure eight, etc.)  
- **Switch and button input** for selecting modes  
- **LCD feedback** for displaying car state and mode selection  
- Modular code structure for easier debugging and extensions  

## Hardware Used  
- **MSP430FR2355 LaunchPad**  
- Motor driver and DC motors  
- Pushbutton switches (for mode selection)  
- Onboard LCD display  
- Power supply (battery pack or bench supply)  

## How It Works  
1. On startup, the car initializes ports, timers, and LCD.  
2. User selects a driving mode using onboard switches.  
3. The FSM executes the selected sequence (e.g., straight, circle, triangle).  
4. Car provides feedback via the LCD.  
5. Timer interrupts manage motor PWM and state transitions.  

## Example Modes  
- **Straight:** Car drives forward for a set distance  
- **Circle:** Car drives in a circular path  
- **Triangle:** Car completes a triangular path  
- **Figure Eight:** Car drives in an infinity-shaped pattern  

## Web Joystick Teleop
- Open `web/joystick.html` in any modern browser (or host it on a phone/tablet) to get a virtual thumb-stick.
- Set the car's Wi-Fi endpoint (default `http://192.168.4.1:8080`) and click **Save**. The UI persists the value locally.
- Push up to accelerate, drift the knob left/right to differentially trim each wheel, and release anywhere to stop.
- The page streams `application/x-www-form-urlencoded` POSTs to `/api/joystick` with fields `x`, `y`, and `active` expressed in thousandths (±1000). The firmware enforces the PWM safety window defined in `motors.h`, so future recalibration is a matter of editing those constants.
- Use `GET /health` for a quick JSON heartbeat (SSID, IP, failsafe ticks) or `OPTIONS /api/joystick` for CORS preflight checks.
