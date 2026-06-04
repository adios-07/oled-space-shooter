#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define I2C_ADDRESS 0x3C
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial stm32(2, 3); 


uint32_t frame_count = 0;

char serialBuf[24];
uint8_t serialIdx = 0;
int32_t encoder_val = 0;
uint8_t button_pulse = 0;
bool has_init_data = false; 

// --- GAME SPECS ---
#define PLAYER_W      11
#define PLAYER_Y      55
#define PLAYER_SENSI   2
#define MAX_BULLETS    3
#define MAX_ENEMIES    5
#define ENEMY_W        8
#define ENEMY_H        4
#define BULLET_SPD     4
#define ENEMY_SPD      1
#define SPAWN_RATE    55

struct Bullet { int16_t x, y; bool active; };
struct Enemy  { int16_t x, y; bool active; };

int16_t  player_x;
int32_t  last_enc;
Bullet   bullets[MAX_BULLETS];
Enemy    enemies[MAX_ENEMIES];

uint16_t score;
uint8_t  spawnTimer;
bool     gameOver;
uint32_t lastFrame = 0;

void readSerial() {
    while (stm32.available()) {
        char c = stm32.read();
        if (c == '\n') {
            serialBuf[serialIdx] = '\0';
            serialIdx = 0;
            char *semi = strchr(serialBuf, ';');
            if (semi) {
                *semi = '\0';
                encoder_val = atol(serialBuf);
                
                if (atoi(semi + 1) == 1) {
                    button_pulse = 1; 
                }

                if (!has_init_data) {
                    last_enc = encoder_val;
                    has_init_data = true;
                }
            }
        } else if (c != '\r' && serialIdx < 23) {
            serialBuf[serialIdx++] = c;
        }
    }
}

// --- CORE MECHANICS ---
void spawnEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = random(0, SCREEN_WIDTH - ENEMY_W);
            enemies[i].y = 10;
            enemies[i].active = true;
            return;
        }
    }
}

void fireBullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = player_x + 5;
            bullets[i].y = PLAYER_Y - 2;
            bullets[i].active = true;
            return;
        }
    }
}

void initGame() {
    player_x   = 58;
    last_enc   = encoder_val; 
    score      = 0;
    spawnTimer = 0;
    gameOver   = false;
    button_pulse = 0;
    
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    spawnEnemy();
}

// --- DRAWING FUNCTIONS ---
void drawPlayer(int16_t x) {
    display.drawPixel(x + 5, PLAYER_Y,     SH110X_WHITE);
    display.drawFastHLine(x + 4, PLAYER_Y + 1, 3,  SH110X_WHITE);
    display.drawFastHLine(x + 2, PLAYER_Y + 2, 7,  SH110X_WHITE);
    display.drawFastHLine(x,     PLAYER_Y + 3, 11, SH110X_WHITE);
    display.drawPixel(x + 1, PLAYER_Y + 4, SH110X_WHITE);
    display.drawPixel(x + 5, PLAYER_Y + 4, SH110X_WHITE);
    display.drawPixel(x + 9, PLAYER_Y + 4, SH110X_WHITE);
}

void drawEnemy(int16_t x, int16_t y) {
    display.drawRect(x, y, ENEMY_W, ENEMY_H, SH110X_WHITE);
    display.drawPixel(x + 2, y + 1, SH110X_WHITE);
    display.drawPixel(x + 5, y + 1, SH110X_WHITE);
}

void setup() {
    Serial.begin(9600);
    
    // GIVE OLED HALF A SECOND TO WAKE UP
    delay(500); 
    randomSeed(analogRead(A0));

    if (!display.begin(I2C_ADDRESS, true)) {
        Serial.println(F("OLED init failed!"));
        while (1);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    
    // USE F() MACRO TO SAVE RAM
    display.setCursor(10, 20);
    display.println(F("  SPACE SHOOTER"));
    display.setCursor(10, 35);
    display.println(F("Waiting for STM32..."));
    display.display();

    stm32.begin(9600);
    initGame();
}

void loop() {
    readSerial(); 

    if (!has_init_data) return; 

    uint32_t now = millis();
    if (now - lastFrame < 33) return; 
    lastFrame = now;
    frame_count++;

    // --- GAME OVER LOGIC ---
    if (gameOver) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(8, 10);
        display.println(F("GAME OVER"));
        display.setTextSize(1);
        display.setCursor(35, 35);
        display.print(F("Score: "));
        display.println(score);
        display.setCursor(10, 50);
        display.println(F("Click to restart"));
        display.display();

        if (button_pulse) {
            initGame();
        }
        return;
    }

    // --- PLAYING LOGIC ---
    // 1. Move player
    int32_t delta = encoder_val - last_enc;
    last_enc = encoder_val;
    player_x += (int16_t)(delta * PLAYER_SENSI);
    player_x  = constrain(player_x, 0, SCREEN_WIDTH - PLAYER_W);

    // 2. Fire on button press
    if (button_pulse) {
        fireBullet();
        button_pulse = 0;
    }

    // 3. Spawn enemies
    spawnTimer++;
    if (spawnTimer >= SPAWN_RATE) {
        spawnEnemy();
        spawnTimer = 0;
    }

    // 4. Move bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].y -= BULLET_SPD;
        if (bullets[i].y < 10) bullets[i].active = false;
    }

    // 5. Move enemies & Check Game Over
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if(frame_count % 2 == 0)
          enemies[i].y += ENEMY_SPD;
        
        if (enemies[i].y + ENEMY_H >= PLAYER_Y) {
            gameOver = true;
            return;
        }
    }

    // 6. Check Collisions (Bullet hits Enemy)
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active) continue;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!enemies[e].active) continue;
            
            if (bullets[b].x >= enemies[e].x &&
                bullets[b].x <= enemies[e].x + ENEMY_W &&
                bullets[b].y >= enemies[e].y &&
                bullets[b].y <= enemies[e].y + ENEMY_H) {
                
                bullets[b].active = false;
                enemies[e].active = false;
                score++;
            }
        }
    }

    // --- RENDER ---
    display.clearDisplay();
    
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("SCORE:"));
    display.print(score);
    display.drawFastHLine(0, 8, SCREEN_WIDTH, SH110X_WHITE);
    
    drawPlayer(player_x);
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) display.drawFastVLine(bullets[i].x, bullets[i].y, 4, SH110X_WHITE);
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) drawEnemy(enemies[i].x, enemies[i].y);
    }
    
    display.display();
}