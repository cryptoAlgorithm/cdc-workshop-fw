#include "lcd.h"

#include "Arduino.h"

LiquidCrystal lcd(11, 12, 2, 3, 4, 5); // 

static uint8_t alert_visible = 0;

const static uint8_t custom_edges[][8] = {{
  0b00000,
  0b00000,
  0b00000,
  0b00111,
  0b00100,
  0b00100,
  0b00100,
  0b00100
}, {
  0b00000,
  0b00000,
  0b00000,
  0b11100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
}, {
  0b00100,
  0b00100,
  0b00100,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000
}, {
  0b00100,
  0b00100,
  0b00100,
  0b11100,
  0b00000,
  0b00000,
  0b00000,
  0b00000
}};

const static uint8_t custom_hb_sm_char[] = {
  0b00000,
  0b00000,
  0b01010,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
}, custom_hb_lg_char[] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000
};

static const uint8_t custom_tick_char[] = {
  0b00000,
  0b00000,
  0b00001,
  0b00011,
  0b10110,
  0b11100,
  0b01000,
  0b00000
};

void lcd_init() {
  lcd.begin(20, 4);
  // Register custom chars
  uint8_t cust_char[8];
  for (uint8_t i = 1; i < 2; ++i) { // save memory by generating char map in runtime
    memset(cust_char, ((0b11111) >> i) << i, sizeof(cust_char));
    lcd.createChar(i-1, cust_char);
  }
  lcd.createChar(LCD_CHAR_TICK, (uint8_t *) custom_tick_char);
  lcd.createChar(LCD_CHAR_HEART_SM, (uint8_t *) custom_hb_sm_char);
  lcd.createChar(LCD_CHAR_HEART_LG, (uint8_t *) custom_hb_lg_char);
  for (uint8_t i = 0; i < 4; ++i) {
    lcd.createChar(LCD_CHAR_TOP_LEFT + i, (uint8_t *) custom_edges[i]);
  }
}

void lcd_hello_seq() {
  lcd_draw_text_center("Welcome to", 1, 0);
  for (uint8_t i = 0; i <= 50; ++i) {
    switch (i) {
      case 0: break;
      case 1:
        lcd.setCursor(19, 2); lcd.write('G');
        break;
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
        lcd.setCursor(20 - (i - 1), 2); lcd.write(' ');
        lcd.setCursor(20 - i, 2); lcd.write('G');
        break;
      case 14:
        lcd.setCursor(19, 2); lcd.write('R');
        break;
      case 15:
      case 16:
      case 17:
      case 18:
      case 19:
      case 20:
      case 21:
      case 22:
      case 23:
      case 24:
      case 25:
        lcd.setCursor(20 - (i - 14), 2); lcd.write(' ');
        lcd.setCursor(20 - (i - 13), 2); lcd.write('R');
        break;
      case 26:
        lcd.setCursor(19, 2); lcd.write('G');
        break;
      case 27:
      case 28:
      case 29:
      case 30:
      case 31:
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
        lcd.setCursor(20 - (i - 26), 2); lcd.write(' ');
        lcd.setCursor(20 - (i - 25), 2); lcd.write('G');
        break;
      case 37:
        lcd.setCursor(19, 2); lcd.write('E');
        break;
      case 38:
      case 39:
      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
      case 45:
      case 46:
        lcd.setCursor(20 - (i - 37), 2); lcd.write(' ');
        lcd.setCursor(20 - (i - 36), 2); lcd.write('E');
        break;
      default: break;
    }
    delay(100);
  }
  lcd_clear();
}

void lcd_draw_text_center(const char * text, const uint8_t row, const uint8_t padding) {
  const uint8_t len = strlen(text);
  uint8_t st = padding;
  if (len <= 20-padding*2) {
    st = (20 - padding*2 - len) / 2 + padding;
  }
  lcd.setCursor(st, row);
  lcd.print(text);
}

uint8_t lcd_can_draw() {
  return alert_visible == 0;
}

void lcd_clear() {
  uint8_t buf[20];
  memset(buf, ' ', sizeof(buf));
  for (uint8_t r = 0; r < 4; ++r) {
    lcd.setCursor(0, r);
    lcd.write(buf, sizeof(buf));
  }
  alert_visible = 0;
}

void lcd_draw_alert(const char * title, const char * sub) {
  if (!lcd_can_draw()) {
    Serial.println(F("Tried to draw alert when cannot draw!"));
    return;
  }

  // Draw solid "border" box - intuitive method but not most efficient
  for (uint8_t x = 0; x < 20; ++x) {
    for (uint8_t y = 0; y < 4; ++y) {
      lcd.setCursor(x, y);
      lcd.write(
        y == 0 && x == 0 ? LCD_CHAR_TOP_LEFT :
        y == 0 && x == 20-1 ? LCD_CHAR_TOP_RIGHT :
        y == 4-1 && x == 0 ? LCD_CHAR_BOTTOM_LEFT :
        y == 4-1 && x == 20-1 ? LCD_CHAR_BOTTOM_RIGHT :
        y == 0 || y == 4-1 ? '-' :
        x == 0 || x == 20-1 ? '|' :
        ' '
      );
    }
  }

  lcd_draw_text_center(title, 1, 1);
  if (sub) lcd_draw_text_center(sub, 2, 1);

  alert_visible = 1;
}



void lcd_update_result() {

}
