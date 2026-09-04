/*
 * render.h — 描画処理の公開窓口
 * --------------------------------------------------------------------------
 * main.cから呼ぶ初期化・描画準備・VBlank反映の3関数を宣言。
 * renderGame()はconst Game *を受け取るため、表示のためにゲーム状態を
 * 書き換えないことを関数の型で約束している。
 *
 * VRAM、OAM、VBlankはBlocksDS/libnds固有の内容です。
 */
#ifndef RENDER_H
#define RENDER_H

#include "game_types.h"

/* 起動時に1回だけ、画面モード・VRAM・スプライト画像を準備する。 */
void renderInit(void);
/* 現在のGameから背景・駒・カーソル・下画面を描画用メモリへ用意する。 */
void renderGame(const Game *game);
/* VBlank中にOAMの内容を実画面へ反映する。 */
void renderVBlank(void);

#endif /* RENDER_H */
