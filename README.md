# oled-space-shooter
A two-microcontroller Space Shooter game. An STM32 board processes rotary encoder and button inputs using hardware timers, streaming the data via UART to an Arduino. The Arduino handles the game logic and renders the graphics in real-time on a 128x64 I2C OLED display.
