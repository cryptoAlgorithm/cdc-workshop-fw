#pragma once

#include <LiquidCrystal.h>

extern LiquidCrystal lcd;

typedef enum {
  LCD_CHAR_BLOCK_4,
  LCD_CHAR_TICK,
  LCD_CHAR_HEART_SM,
  LCD_CHAR_HEART_LG,
  LCD_CHAR_TOP_LEFT,
  LCD_CHAR_TOP_RIGHT,
  LCD_CHAR_BOTTOM_LEFT,
  LCD_CHAR_BOTTOM_RIGHT
} lcd_custom_char_t;

void lcd_init();

void lcd_update_result();

void lcd_draw_text_center(const char * text, const uint8_t row, const uint8_t padding);

void lcd_draw_alert(const char * title, const char * sub);

uint8_t lcd_can_draw();

void lcd_clear();
