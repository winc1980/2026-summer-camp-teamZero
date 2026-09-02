/*
 * board.h
 * 盤面の境界・地形・占有・移動・攻撃範囲を判定する関数の公開窓口。
 * game.cは判定式を重複して持たず、ここへ質問する形にしている。
 */
#ifndef BOARD_H
#define BOARD_H

#include "game_types.h"

/* 全地形を初期状態へする。Gameを書き換えるのでconstなしのポインタ。 */
void boardInit(Game *game);
/* (x,y)が8×6盤面の内側ならtrue。配列境界事故を防ぐ（5・11章）。 */
bool boardIsInside(int x, int y);
/* 指定地形へ着地できるか。現在はPLAINだけtrue。 */
bool boardTerrainIsWalkable(TerrainType terrain);
/* 指定マスにいる生存ユニットの配列番号を返す。空きなら-1。 */
int boardUnitAt(const Game *game, int x, int y);
/* 指定ユニットが目的マスへ移動可能か。 */
bool boardCanMoveTo(const Game *game, int unitIndex, int x, int y);
/* 2体の位置・所有者・攻撃者の種類から攻撃可能か判定する。 */
bool boardCanAttack(const Game *game, int attackerIndex, int targetIndex);
/* 攻撃可能な最初の敵番号を返す。いなければ-1。 */
int boardFindFirstAttackTarget(const Game *game, int attackerIndex);

#endif /* BOARD_H */
