/*
 * render.h
 * Gameの内容をDSの上下画面へ表示する公開関数。
 * const Game *を受け取るため、描画処理はゲーム状態を変更しない（4章）。
 * VRAM/OAM/VBlankの具体的な使い方は事前資料外のBlocksDS固有知識。
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
