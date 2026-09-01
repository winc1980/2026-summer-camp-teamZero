#include <nds.h>
#include <stdio.h>

int main(int argc, char **argv) {
  defaultExceptionHandler();
  consoleDemoInit();

  printf("Hello, DS!\n\n");
  printf("A ボタン: カウント\n");
  printf("START:    終了\n");

  int count = 0;

  while (1) {
    scanKeys();
    u32 down = keysDown();

    if (down & KEY_A)
      count++;
    if (down & KEY_START)
      break;

    swiWaitForVBlank();

    consoleSetCursor(NULL, 0, 8);
    printf("count = %3d", count);
  }
  return 0;
}