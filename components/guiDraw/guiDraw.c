#include "guiDraw.h"
#include "bitmapa.h"
#include "math.h"

void gui_ekran(Urzadzenie_t* gorne_pole, Urzadzenie_t* srodkowe_pole, Urzadzenie_t* dolne_pole, uint8_t currentItemNum, uint8_t allItemsCount){
  oled_clear();
  oled_draw_bitmap(0,0, selector, 128, 64);
  
  if(srodkowe_pole != NULL){
    oled_printf_xy(10, 28, "%s", srodkowe_pole->nazwa);
    if(srodkowe_pole->przelaczany){
      oled_printf_xy(90, 28, "%d[%d]", srodkowe_pole->stan, srodkowe_pole->wybranaWartosc);
    } else {
      uint8_t szerokosc = floor(log10(abs(srodkowe_pole->wybranaWartosc))) + 1;
      if(srodkowe_pole->wybranaWartosc < 0) szerokosc++;
      oled_printf_xy(90-szerokosc*5, 28, "%d[%d]", srodkowe_pole->stan, srodkowe_pole->wybranaWartosc);
    }
  }
  
  if(allItemsCount > 1){
    if(gorne_pole != NULL){
      oled_printf_xy(8, 6, "%s", gorne_pole->nazwa);
      oled_printf_xy(88, 6, "%d", gorne_pole->stan);
    }
    
    if(dolne_pole != NULL){
      oled_printf_xy(8, 50, "%s", dolne_pole->nazwa);
      oled_printf_xy(88, 50, "%d", dolne_pole->stan);
    }
  }

  gui_scroll(currentItemNum, allItemsCount);
  // oled_printf_xy(1, 57, "[%u / %u]", currentItemNum, allItemsCount); 
}

void gui_scroll(uint8_t currentItemNum, uint8_t allItemsCount){
  if(allItemsCount == 0) return;
  uint8_t wys = (1.0 / allItemsCount) * 64;
  uint8_t start_y = wys * currentItemNum;
  oled_draw_rectangle(126, start_y, 1, wys);
}