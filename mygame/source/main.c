#include <nds.h>
#include "forest_town.h"
#include "button.h"

void drawCell(u16* mapmemory, int x, int y, int type) {
    int starttile = type*16;
    int startx = x*4 + 4; 
    int starty = y*4;
    int tilenumber = 0;
    for (int i = 0; i<4; i++) {
        for (int j = 0; j < 4; j++) {
            mapmemory[(starty+i)*32 +(startx+j)] =starttile + tilenumber;
            tilenumber++;
        }
    }
}

int main(int argc, char *argv[], char **argv2)
{
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    vramSetPrimaryBanks(VRAM_A_MAIN_BG, VRAM_B_LCD, VRAM_C_SUB_BG, VRAM_D_LCD);

    int bg = bgInit(3, BgType_Text8bpp, BgSize_T_256x256, 0, 1);
    dmaCopy(forest_townTiles, bgGetGfxPtr(bg), forest_townTilesLen);
    dmaCopy(forest_townPal, BG_PALETTE, forest_townPalLen);

    u16* mapMemory = bgGetMapPtr(bg);

    for (int i = 0; i < 32*32; i++) {
        mapMemory[i] = 0;
    }

    int map[6][6] = {
        {1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1},
        {1, 2, 2, 2, 1, 1},
        {1, 1, 2, 1, 1, 1},
        {1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1}
    };

    for (int y = 0; y < 6; y++){
        for (int x = 0; x < 6; x++){
            drawCell(mapMemory, x, y, map[y][x]);
        }
    }

    bgShow(bg);

    int x = 0;
    int y = 0;
    
    int bgBtn = bgInitSub(0, BgType_Text8bpp, BgSize_T_256x256, 0, 1);
    dmaCopy(buttonTiles, bgGetGfxPtr(bgBtn), buttonTilesLen);
    dmaCopy(buttonMap, bgGetMapPtr(bgBtn), buttonMapLen);
    dmaCopy(buttonPal, BG_PALETTE_SUB, buttonPalLen);
    
    int btnx = 0;
    int btny = 0;
   
    touchPosition touch;

    while (1)
  {
      swiWaitForVBlank();

      bgSetScroll(bg, x, y);
      bgUpdate();
      scanKeys();

      u16 keys_down = keysDown(); 

      if (keys_down & KEY_TOUCH) {
          touchRead(&touch); 
          if (touch.px >= btnx && touch.px <= (btnx + 256) &&
              touch.py >= btny && touch.py <= (btny + 192)) {
          }
      }
  } 
  return 0;
}