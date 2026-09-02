/*
 * unit.c
 * キャラクターの初期値と、種類に固有の値を決める。
 * 移動ルールはboard.c、ターン進行はgame.cに分離し、このファイルには
 * 「1体そのもの」の知識だけを置く。
 *
 * 事前資料との対応:
 * - 3章「関数まわり」: 引数・戻り値・switchを使った関数分割
 * - 4章「ポインタ」  : Unit *unitが呼び出し元の構造体を直接更新する
 * - 6章「構造体」    : unit->hpのようなアロー演算子
 */
#include "unit.h"

/* キャラクター種類を受け取り、仕様で決めた固定攻撃力を返す。 */
int unitAttackForType(UnitType type)
{
    /* switchは1つの値に応じて処理を分岐する。各caseはenum値に対応する。 */
    switch (type) {
        case UNIT_A: return 60;
        case UNIT_B: return 70;
        case UNIT_C: return 40;
        /* 想定外の値が来ても未定義の値を返さないための安全策。 */
        default: return 0;
    }
}

/*
 * 渡されたUnitを「試合開始時の生存・未行動状態」にする。
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

/* enumが0,1,2の連番であることを利用し、表示文字A,B,Cへ変換する。 */
char unitTypeLetter(UnitType type)
{
    /* キャスト(int)でenumを整数として足し、(char)で文字型へ戻す（2章）。 */
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
