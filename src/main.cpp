#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include <SerialFlash.h>
// #include <ShiftRegister74HC595.h>
#include "config.h"

#define BITS 8

#define SER PC0
#define RCLK PD3
#define SRCLK PD2
#define SRCLR PD0

#define DATA PC0
#define CLK PD0
#define LOAD PD3

// #define SAVE_TRANSLATION_MATRIX
#define SAVE_FONTS

static void set(uint8_t b)
{
  int i;

  for (i = BITS - 1; i >= 0; --i)
  {
    digitalWrite(SER, (b & (1 << i)) != 0 ? HIGH : LOW);
    digitalWrite(SRCLK, HIGH);
    digitalWrite(SRCLK, LOW);
  }
}

void refresh()
{
  digitalWrite(RCLK, HIGH);
  digitalWrite(RCLK, LOW);
}


#define ROWS 15
#define COLS 15
#define LED_ROWS 8
#define LED_COLS 8

#define TRANSLATION_MATRIX_ADDR 0x1000

#define PIN_FLASH_CS PC4
SerialFlashChip flash;

static uint8_t imageBuffer[ROWS][COLS];

#ifdef SAVE_TRANSLATION_MATRIX
static const int8_t globalTranslationMatrix[ROWS][COLS][2] = {
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 0,  0}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 1,  0}, {-1, -1}, { 0,  1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 2,  0}, {-1, -1}, { 1,  1}, {-1, -1}, { 0,  2}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 3,  0}, {-1, -1}, { 2,  1}, {-1, -1}, { 1,  2}, {-1, -1}, { 0,  3}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, { 4,  0}, {-1, -1}, { 3,  1}, {-1, -1}, { 2,  2}, {-1, -1}, { 1,  3}, {-1, -1}, { 0,  4}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, { 5,  0}, {-1, -1}, { 4,  1}, {-1, -1}, { 3,  2}, {-1, -1}, { 2,  3}, {-1, -1}, { 1,  4}, {-1, -1}, { 0,  5}, {-1, -1}, {-1, -1}},
  {{-1, -1}, { 6,  0}, {-1, -1}, { 5,  1}, {-1, -1}, { 4,  2}, {-1, -1}, { 3,  3}, {-1, -1}, { 2,  4}, {-1, -1}, { 1,  5}, {-1, -1}, { 0,  6}, {-1, -1}},
  {{ 7,  0}, {-1, -1}, { 6,  1}, {-1, -1}, { 5,  2}, {-1, -1}, { 4,  3}, {-1, -1}, { 3,  4}, {-1, -1}, { 2,  5}, {-1, -1}, { 1,  6}, {-1, -1}, { 0,  7}},
  {{-1, -1}, { 7,  1}, {-1, -1}, { 6,  2}, {-1, -1}, { 5,  3}, {-1, -1}, { 4,  4}, {-1, -1}, { 3,  5}, {-1, -1}, { 2,  6}, {-1, -1}, { 1,  7}, {-1, -1}},
  {{-1, -1}, {-1, -1}, { 7,  2}, {-1, -1}, { 6,  3}, {-1, -1}, { 5,  4}, {-1, -1}, { 4,  5}, {-1, -1}, { 3,  6}, {-1, -1}, { 2,  7}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, { 7,  3}, {-1, -1}, { 6,  4}, {-1, -1}, { 5,  5}, {-1, -1}, { 4,  6}, {-1, -1}, { 3,  7}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 7,  4}, {-1, -1}, { 6,  5}, {-1, -1}, { 5,  6}, {-1, -1}, { 4,  7}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 7,  5}, {-1, -1}, { 6,  6}, {-1, -1}, { 5,  7}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 7,  6}, {-1, -1}, { 6,  7}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}},
  {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, { 7,  7}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}}

};

static void saveTranslationMatrix(const void * addr)
{
    flash.eraseSector(TRANSLATION_MATRIX_ADDR);
    flash.write(TRANSLATION_MATRIX_ADDR, addr, sizeof(globalTranslationMatrix));
}

#endif

static int8_t translationMatrix[ROWS][COLS][2] = { 0 };

static void loadTranslationMatrix(void * addr)
{
    flash.read(TRANSLATION_MATRIX_ADDR, addr, sizeof(translationMatrix));
}

#define FONTS_ADDR 0x2000
#define IMAGE_SIZE (ROWS * COLS)

#ifdef SAVE_FONTS

#include "font.h"

static void saveFont(const void * addr, uint8_t idx)
{
    flash.write(FONTS_ADDR + IMAGE_SIZE * idx, addr, IMAGE_SIZE);
}

#endif

static void loadImage(void * addr, uint32_t flashAddr)
{
    flash.read(flashAddr, addr, IMAGE_SIZE);
}

static void blink(int n)
{
    int i;

    for (i = 0; i < n; ++i)
    {
        digitalWrite(PC3, HIGH);
        delay(100);
        digitalWrite(PC3, LOW);
        delay(100);
    }
}

void setup()
{
    pinMode(PC3, OUTPUT);

    Serial.begin(115200);

    if (!flash.begin(PIN_FLASH_CS))
    {
        Serial.println(F("SPI Flash not detected."));
        // blink(3);
        while (1)
            ;
    }

    pinMode(SER, OUTPUT);
    pinMode(SRCLK, OUTPUT);
    pinMode(RCLK, OUTPUT);
    pinMode(SRCLR, OUTPUT);

    // Start LOW
    digitalWrite(SRCLK, LOW);
    digitalWrite(RCLK, LOW);

    // Reset first
    digitalWrite(SRCLR, LOW);
    delay(10);
    digitalWrite(SRCLR, HIGH);

    digitalWrite(PC3, HIGH);
    delay(100);
    digitalWrite(PC3, LOW);

#if 0
    // Initialize image
    for (int i = 0; i < 8; ++i)
    {
        imageBuffer[i][8 - i - 1] = 1;
        imageBuffer[i][i] = 1;
    }
#endif

#ifdef SAVE_TRANSLATION_MATRIX
    saveTranslationMatrix(globalTranslationMatrix);
#endif

#ifdef SAVE_FONTS
    // flash.eraseSector(FONTS_ADDR);
    // saveFont(FONT_A, 0);
    // saveFont(FONT_B, 1);
    // saveFont(FONT_C, 2);
    saveFont(FONT_D, 3);
#endif

    loadTranslationMatrix(translationMatrix);

#if 0
    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLS; ++j)
        {
            char s[16];
            if ((translationMatrix[i][j][0] == -1) || (translationMatrix[i][j][1] == -1))
            {
                sprintf(s, "       ");
            }
            else
            {
                sprintf(s, " (%d,%d) ", translationMatrix[i][j][0], translationMatrix[i][j][1]);
            }
            Serial.print(s);
        }

        Serial.println("");
    }

    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLS; ++j)
        {
            char s[16];

            if (imageBuffer[i][j])
            {
                sprintf(s, "* ");
            }
            else
            {
                sprintf(s, "  ");
            }

            Serial.print(s);
        }

        Serial.println("");
    }
#endif

    blink(1);
}

static void drawDisplayBuffer()
{
    uint8_t rows[LED_ROWS] = { 0 };

    for (int i = 0; i < ROWS; ++i)
    {
        for (int j = 0; j < COLS; ++j)
        {
            int x = translationMatrix[i][j][0];
            int y = translationMatrix[i][j][1];

            if ((x == -1) || (y == -1))
            {
                continue;
            }

            if (!imageBuffer[i][j])
            {
                continue;
            }

            rows[y] |= 1 << x;
        }
    }

    for (int i = 0; i < LED_COLS; ++i)
    {
        set(~(1 << i));
        set(rows[i]);
        refresh();
    }
}

static uint8_t c = 0;
static uint8_t letter = 0;

void loop()
{
    if (c++ == 255)
    {
        letter++;
    }

    letter = letter % NUM_LETTERS;

    loadImage(imageBuffer, FONTS_ADDR + IMAGE_SIZE * letter);

    drawDisplayBuffer();
}
