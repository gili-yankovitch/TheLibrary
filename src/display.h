#ifndef __DISPLAY_H__
#define __DISPLAY_H__

// #define SAVE_TRANSLATION_MATRIX
#define SAVE_FONTS

#define ROWS 15
#define COLS 15
#define LED_ROWS 8
#define LED_COLS 8

#define TRANSLATION_MATRIX_ADDR 0x1000

void initDisplay();
void drawDisplayBuffer();

#endif // __DISPLAY_H__
