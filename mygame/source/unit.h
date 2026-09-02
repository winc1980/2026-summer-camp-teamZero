/*
 * unit.h
 * キャラクター1体の初期化と、種類ごとの基本値を外部へ公開するヘッダ。
 * 宣言と実装を.h/.cへ分ける考え方は1章「#includeの正体」と3章「関数」。
 */
#ifndef UNIT_H
#define UNIT_H

/* Unit、UnitType、Playerの定義を使うために読み込む。 */
#include "game_types.h"

/* Unit構造体の各メンバへ初期値を代入する。Unit * は4章のポインタ。 */
void unitInit(Unit *unit, UnitType type, Player owner, int x, int y);
/* A・B・Cごとの攻撃力を返す。 */
int unitAttackForType(UnitType type);
/* 画面表示用にUNIT_A/UNIT_B/UNIT_Cを'A'/'B'/'C'へ変換する。 */
char unitTypeLetter(UnitType type);
/* 文字列として種類名を返す。現在はデバッグ・拡張用。 */
const char *unitTypeName(UnitType type);

#endif /* UNIT_H */
