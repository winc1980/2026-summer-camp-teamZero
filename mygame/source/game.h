/*
 * game.h
 * ゲーム状態の初期化と、1フレーム分の更新という2つの公開関数だけを定義する。
 * main.cは内部の状態遷移を知らず、この窓口だけを呼ぶ（3章・10章）。
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
