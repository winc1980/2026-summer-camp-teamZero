/*
 * game.h — ゲーム進行処理の公開窓口
 * --------------------------------------------------------------------------
 * main.cなど、game.cの外から呼んでよい関数を宣言しています。
 * 内部の細かな状態遷移を公開せず、「初期化」と「1フレーム更新」の2つに
 * 絞ることで、呼び出し側がゲーム進行の詳細へ依存しないようにしています。
 *
 * 参考: 事前資料 1章（ヘッダ）、3章（関数宣言）、10章（更新処理の分離）
 */
#ifndef GAME_H
#define GAME_H

#include "game_types.h"
#include "input.h"

/* 1試合分のGameを初期状態にする。 */
void gameInit(Game *game);
/* 現在状態と1フレームの入力から、次のゲーム状態へ進める。 */
void gameUpdate(Game *game, GameInput input);

#endif /* GAME_H */
