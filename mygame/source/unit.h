/*
 * unit.h — キャラクター基本情報の公開窓口
 * --------------------------------------------------------------------------
 * Unitの初期化と、A/B/Cごとの値・表示名を取得する関数を宣言しています。
 * 関数の実装はunit.cに置き、利用側はこのヘッダだけを#includeします。
 *
 * 参考: 事前資料 1章（.hと.cの分割）、3章（関数）、4章（ポインタ）
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
