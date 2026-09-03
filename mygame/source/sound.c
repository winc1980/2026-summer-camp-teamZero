#include <maxmod9.h>
#include <nds.h>

#include "soundbank.h"
#include "soundbank_bin.h"

void playBGM() { mmStart(MOD_EASY8BIT_20260903_1149, MM_PLAY_LOOP); }

void stopBGM() { mmStop(); }

/*BGMの再生状況をリセットする。リセットするだけで流しなおしてはくれないので、流すときにplayBGM()を呼び出す必要あり*/
void resetBGM() {
  mmUnload(MOD_EASY8BIT_20260903_1149);
  mmLoad(MOD_EASY8BIT_20260903_1149);
}

/*音楽を流すのに必要なプレイヤーのinitialize*/
static void setUpPlayer() {
  soundEnable();
  mmInitDefaultMem((mm_addr)soundbank_bin);
}

/*bgmを設定して流しはじめる*/
static void bgmInit() {
  mmLoad(MOD_EASY8BIT_20260903_1149);
  playBGM();
}

/*main関数実行開始時の一回のみ呼び出す。*/
void soundInit() {
  setUpPlayer();
  bgmInit();
}