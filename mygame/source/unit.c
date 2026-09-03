/*
 * unit.c — キャラクター1体の初期値と種類別の基本値
 * --------------------------------------------------------------------------
 * A/B/Cの攻撃力と、試合開始時のUnit構造体を設定。
 * 移動・攻撃範囲はboard.c、ターンやダメージ処理はgame.cへ分けてるため、
 * このファイルには「キャラクター1体が最初に持つ値」を集めている。
 *
 * 変更時の目安:
 *  A/B/Cの攻撃力を調整するときはunitAttackForType()を変更する
 *  全キャラクター共通の初期HPはgame_types.hのINITIAL_HPを変更
 *  種類を増やす場合はUnitType、表示、盤面ルール、テストも合わせて更新
 *
 * 参考資料:
 *  事前資料 3章: 引数、戻り値、switchによる関数分割
 *  事前資料 4章: Unit *で呼び出し元の構造体を更新する
 *  事前資料 6章: Unit構造体とunit->hp形式のメンバアクセス
 */
#include "unit.h"

/* キャラクター種類を受け取り、仕様で決めた固定攻撃力を返す。 */
int unitAttackForType(UnitType type)
{
    /* switchは1つの値に応じて処理を分岐する。各caseはenum値に対応。 */
    switch (type) {
        case UNIT_A: return 60;
        case UNIT_B: return 70;
        case UNIT_C: return 40;
        /* 想定外の値が来ても未定義の値を返さないための安全策。 */
        default: return 0;
    }
}

/*
 * 渡されたUnitを「試合開始時の生存・未行動状態」にる。
 * unitはアドレスを受け取るポインタなので、この代入は関数を抜けた後も残る。
 */
void unitInit(Unit *unit, UnitType type, Player owner, int x, int y)
{
    /* ポインタ先の構造体メンバは「unit->type」のように->で指定する（4・6章）。 */
    unit->type = type;
    unit->owner = owner;
    unit->x = x;
    unit->y = y;
    unit->hp = INITIAL_HP;
    unit->attack = unitAttackForType(type);
    unit->alive = true;
    unit->acted = false;
}

/* enumが0,1,2の連番であることを利用して、表示文字A,B,Cへ変換する。 */
char unitTypeLetter(UnitType type)
{
    /* キャストでenumを整数として足し、(char)で文字型へ戻す（2章）。 */
    return (char)('A' + (int)type);
}

/* const char * は「変更しない文字列の先頭アドレス」を表す（4・5章）。 */
const char *unitTypeName(UnitType type)
{
    switch (type) {
        case UNIT_A: return "A";
        case UNIT_B: return "B";
        case UNIT_C: return "C";
        default: return "?";
    }
}

/* 正式な技名が決まるまでの仮名。後からこの一覧だけを差し替えればよい。 */
const char *unitSkillName(UnitType type)
{
    switch (type) {
        case UNIT_A: return "SKILL A";
        case UNIT_B: return "SKILL B";
        case UNIT_C: return "SKILL C";
        default: return "SKILL ?";
    }
}
