#include <Arduino.h>
#include <SerialFlash.h>
#include "display.h"

#define BITS 8
#define SER PC0
#define RCLK PD3
#define SRCLK PD2
#define SRCLR PD0

extern SerialFlashChip flash;

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

void saveFont(const void * addr, uint8_t idx)
{
    flash.write(FONTS_ADDR + IMAGE_SIZE * idx, addr, IMAGE_SIZE);
}

#endif

void loadImage(void * addr, uint32_t flashAddr)
{
    flash.read(flashAddr, addr, IMAGE_SIZE);
}

void initDisplay()
{
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

#ifdef SAVE_TRANSLATION_MATRIX
    saveTranslationMatrix(globalTranslationMatrix);
#endif

#ifdef SAVE_FONTS
    // flash.eraseSector(FONTS_ADDR);
    // saveFont(FONT_A, 0);
    // saveFont(FONT_B, 1);
    // saveFont(FONT_C, 2);
    // saveFont(FONT_D, 3);
    // saveFont(FONT_E, 4);
    // saveFont(FONT_F, 5);
    // saveFont(FONT_G, 6);
    // saveFont(FONT_H, 7);
    // saveFont(FONT_I, 8);
    // saveFont(FONT_J, 9);
    // saveFont(FONT_K, 10);
    // saveFont(FONT_L, 11);
    // saveFont(FONT_M, 12);
    // saveFont(FONT_N, 13);
    // saveFont(FONT_O, 14);
    // saveFont(FONT_P, 15);
    // saveFont(FONT_Q, 16);
    // saveFont(FONT_R, 17);
    // saveFont(FONT_S, 18);
    // saveFont(FONT_T, 19);
    // saveFont(FONT_U, 20);
    // saveFont(FONT_V, 21);
    // saveFont(FONT_W, 22);
    // saveFont(FONT_X, 23);
    // saveFont(FONT_Y, 24);
    // saveFont(FONT_Z, 25);
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
}

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

static void refresh()
{
  digitalWrite(RCLK, HIGH);
  digitalWrite(RCLK, LOW);
}

void drawDisplayBuffer()
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
