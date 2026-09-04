/*
 * board.h — 盤面ルール判定の公開窓口
 * --------------------------------------------------------------------------
 * game.c、render.c、テストから利用する判定関数を宣言している。
 * 判定式を複数ファイルへ重複させず、実際のルールはboard.cだけで管理する。
 * そのため、ルールを変更するとゲーム進行・範囲表示・テストへ同じ結果が
 * 反映される。
 *
 */
#ifndef BOARD_H
#define BOARD_H

#include "game_types.h"

/* 全地形を初期状態にする。Gameを書き換えるのでconstなしのポインタ。 */
void boardInit(Game *game);
/* (x,y)が8×6盤面の内側ならtrue。配列境界事故を防ぐ。 */
bool boardIsInside(int x, int y);
/* 指定地形へ着地できるか。現在はPLAINだけtrue。 */
bool boardTerrainIsWalkable(TerrainType terrain);
/* 指定マスにいる生存ユニットの配列番号を返す。空きなら-1。 */
int boardUnitAt(const Game *game, int x, int y);
/* 指定ユニットが目的マスへ移動可能か。 */
bool boardCanMoveTo(const Game *game, int unitIndex, int x, int y);
/* 指定位置から対象マスへ攻撃が届くか確認。移動先プレビューからも利用できる。 */
bool boardCanAttackFrom(const Game *game, int attackerIndex, int fromX, int fromY,
                        int targetX, int targetY);
/* 2体の位置・所有者・攻撃者の種類から攻撃可能か判定。 */
bool boardCanAttack(const Game *game, int attackerIndex, int targetIndex);
/* 攻撃可能な最初の敵番号を返す。いなければ-1。 */
int boardFindFirstAttackTarget(const Game *game, int attackerIndex);

#endif /* BOARD_H */
