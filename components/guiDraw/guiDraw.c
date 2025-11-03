#include "guiDraw.h"
#include "bitmapa.h"


void gui_ekran(Pole_t* gorne_pole, Pole_t* srodkowe_pole, Pole_t* dolne_pole, uint8_t currentItemNum, uint8_t allItemsCount){
  oled_clear();
  oled_draw_bitmap(0,0, selector, 128, 64);
  if(gorne_pole != NULL){
    oled_printf_xy(8, 6, "%s", gorne_pole->nazwa);
    oled_printf_xy(98, 6, "%u", gorne_pole->stan);
  }
  
  if(srodkowe_pole != NULL){
    oled_printf_xy(10, 28, "%s", srodkowe_pole->nazwa);
    oled_printf_xy(100, 28, "%u", srodkowe_pole->stan);
  }

  if(dolne_pole != NULL){
    oled_printf_xy(8, 50, "%s", dolne_pole->nazwa);
    oled_printf_xy(98, 50, "%u", dolne_pole->stan);
  }
  gui_scroll(currentItemNum, allItemsCount);
  // oled_printf_xy(1, 57, "[%u / %u]", currentItemNum, allItemsCount); 
}

void gui_scroll(uint8_t currentItemNum, uint8_t allItemsCount){
  if(allItemsCount == 0) return;
  uint8_t wys = (1.0 / allItemsCount) * 128;
  uint8_t start_y = wys * currentItemNum;
  oled_draw_rectangle(126, start_y, 1, wys);
}