#include "lcd.h"

LiquidCrystal lcd(1, 3, 4, 5, 6, 7);

void lcd_init() {
  lcd.begin(20, 4);
}

void lcd_update_result() {
  
}
