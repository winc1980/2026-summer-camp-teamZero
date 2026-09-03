#ifndef SOUND_H
#define SOUND_H

#include "game_types.h"
void playBGM();

void stopBGM();

/*BGMの再生状況をリセットする。リセットするだけで流しなおしてはくれないので、流すときにplayBGM()を呼び出す必要あり*/
void resetBGM();

void selectSE();

bool hpDecreased(Game *game, Game *preGame);

void playAppropriateSE(Game *game, Game *preGame);

/*main関数実行開始時の一回のみ呼び出す。*/
void soundInit();
#endif /*SOUND_H*/