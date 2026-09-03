#include <maxmod9.h>
#include <nds.h>

#include "game_types.h"
#include "sound.h"
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

static void soundEffectInit() {
  /*勝敗決定時に鳴らす*/
  mmLoadEffect(SFX_SFX_FANFARE_20260903_1442);
  /*その他決定時に鳴らす*/
  mmLoadEffect(SFX_SFX_SELECT_20260902_2133);
}

void playAppropriateSE(Game *game, Game *preGame) {
  /*gameとpreGameがNULLじゃないか判定*/
  if (game != NULL && preGame != NULL) {
    /*phaseが他の何かからPHASE_GAME_OVERに == ゲームが終わった瞬間*/
    if (preGame->phase != PHASE_GAME_OVER && game->phase == PHASE_GAME_OVER)
      mmEffectVolume(mmEffect(SFX_SFX_FANFARE_20260903_1442), 255);
    /*フェーズが変動 == Aで有効な状態遷移*/
    else if (preGame->phase != game->phase)
      mmEffect(SFX_SFX_SELECT_20260902_2133);
  }
}

/*main関数実行開始時の一回のみ呼び出す。*/
void soundInit() {
  setUpPlayer();
  bgmInit();
  soundEffectInit();
}
