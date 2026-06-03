# STM32-Arduino Space Shooter

A hardware-split arcade game where an STM32 acts as the input controller and an Arduino acts as the game engine and display driver. 

## Architecture
1. **STM32 (Controller):** Uses hardware timers (TIM2) to read a rotary encoder with zero CPU overhead. Reads a debounced push-button and streams the input state `(encoder_value;button_pulse)` over USART.
2. **Arduino (Game Engine):** Parses the incoming serial data to move the player and fire bullets. Handles enemy spawning, collision detection, and renders the game on an I2C OLED display.

## Hardware Required
* STM32F103 (Blue Pill) or similar
* Arduino (Uno/Nano)
* Rotary Encoder (with push button)
* 128x64 I2C OLED Display (SH1106)

## Wiring Connections

**STM32 to Inputs:**
* Encoder Pins A/B -> `PA0`, `PA1` (TIM2 Channels 1 & 2)
* Encoder Button -> `PB8` (Pulled up to 3.3V)
* Onboard LED (Feedback) -> `PC13`

**STM32 to Arduino (UART):**
* STM32 `TX` (USART1) -> Arduino Pin `2` (SoftwareSerial RX)
* STM32 `RX` (USART1) -> Arduino Pin `3` (SoftwareSerial TX)
* *Note: Ensure common Ground between both boards.*

**Arduino to OLED:**
* SDA -> Arduino `A4`
* SCL -> Arduino `A5`

## Dependencies
* **STM32:** STM32Cube HAL
* **Arduino:** `Adafruit_GFX`, `Adafruit_SH110X`, `SoftwareSerial`

## How to Run
1. Flash the `main.c` code to the STM32 using STM32CubeIDE.
2. Flash the Arduino sketch using the Arduino IDE.
3. Power both boards. Turn the encoder to move the ship horizontally, and click the encoder button to fire.
