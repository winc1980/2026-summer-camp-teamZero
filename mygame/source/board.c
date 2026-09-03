/*
 * board.c — 盤面上の移動・攻撃が可能かを判定する
 * --------------------------------------------------------------------------
 * 8×6盤面の境界、地形、ユニットの占有、A/B/Cの移動範囲と攻撃範囲を
 * まとめている。このファイルは判定だけを行い、座標やHPを変更しない。
 * （重要！）画像・色・スプライトにも触れないため、見た目を差し替えても盤面ルールは
 * 独立して保たれる。
 *
 * 主な呼び出し関係:
 *  game.cが、移動や攻撃を確定する前にboardCanMoveTo()や
 *   boardCanAttack()へ問い合わせる
 *  render.cが、移動可能範囲・攻撃可能範囲の表示にも同じ判定を使う
 *  test/test_rules.cが、DSを起動せずルールを自動確認する
 *
 * 変更時の目安:
 *  A/B/Cの移動範囲・攻撃範囲はboardCanMoveTo()/boardCanAttack()を変更する
 *  山・川・建造物の通行条件はboardTerrainIsWalkable()から拡張する
 *  terrain[y][x]やunits[i]へ触る前に、必ず境界と配列番号を確認する
 *  ルール変更後はtest/test_rules.cにも対応するケースを追加する
 *
 * 参考資料:
 *  事前資料 3章: 小さな判定関数とstatic関数への分割
 *  事前資料 4章: const Game *で状態をコピーせず読み取る
 *  事前資料 5章: 2次元配列と境界チェック
 *  事前資料 6章: Game内のUnit配列とterrain配列
 *  事前資料 10章: 当たり判定と更新処理の分離
 *  事前資料 11章: 不正な座標・番号を早めに拒否する
 *  A/B/C固有のルールは本作独自の仕様
 */

/* abs()（整数の絶対値）を使うためのC標準ヘッダ。 */
#include <stdlib.h>

#include "board.h"

/* 全マスを草原扱いのPLAINで初期化。 */
void boardInit(Game *game)
{
    /* Cではループ変数を先に宣言できる。xが列、yが行。 */
    int y;
    int x;

    /*
     * 二重forでterrain[y][x]の48マスをすべて走査する（5章）。
     * 外側をyにすると、メモリ上も行ごとに順番にアクセスできる。
     */
    for (y = 0; y < BOARD_HEIGHT; y++) {
        for (x = 0; x < BOARD_WIDTH; x++) {
            game->terrain[y][x] = TERRAIN_PLAIN;
        }
    }
}

/* 配列へ触る前に、xとyが両方とも有効範囲内か確認する。 */
bool boardIsInside(int x, int y)
{
    /* && は全条件がtrueのときだけtrueになる論理積。 */
    return x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT;
}

/* （重要！）将来、山・川・建造物ごとの通行ルールを足す入口。 */
bool boardTerrainIsWalkable(TerrainType terrain)
{
    return terrain == TERRAIN_PLAIN;
}

/* (x,y)に生存中のユニットがいるか、6体の配列を先頭から探す。 */
int boardUnitAt(const Game *game, int x, int y)
{
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        /*
         * &game->units[i]でi番目要素のアドレスを取り、ポインタにする（4章）。
         * const Unit *なので、この関数からユニットを書き換えることはできない。
         */
        const Unit *unit = &game->units[i];
        /* 死亡ユニットは盤面にいないものとして扱う。 */
        if (unit->alive && unit->x == x && unit->y == y) {
            return i;
        }
    }
    /* 有効な配列番号0〜5と重ならない-1を「見つからない」の印にする。 */
    return -1;
}

/*
 * staticを付けると、この.cファイル内からだけ呼べる補助関数になる（3章）。
 * 着地点が通行可能で、別ユニットに占有されていないか確認。
 */
static bool boardCanLand(const Game *game, int unitIndex, int x, int y)
{
    int occupant;

    /* 呼び出し元で盤面内を確認済みなので、安全にterrain[y][x]を読める。 */
    if (!boardTerrainIsWalkable(game->terrain[y][x])) {
        return false;
    }
    occupant = boardUnitAt(game, x, y);
    /* 空き(-1)か、自分自身の現在地なら着地可能。その場待機を許すため。 */
    return occupant < 0 || occupant == unitIndex;
}

/*
 * A/B/Cの移動可能範囲を一か所で判定する中心関数。
 * 「状態を変更せず、質問にboolで答える」関数なので、描画時のハイライトと
 * Aボタン決定時の両方で同じルールを再利用可能。
 */
bool boardCanMoveTo(const Game *game, int unitIndex, int x, int y)
{
    const Unit *unit;
    int dx;
    int dy;
    int forward;

    /*
     * 不正な番号や盤外座標を先に弾き、危険な配列アクセスを防ぐ。
     */
    if (unitIndex < 0 || unitIndex >= UNIT_COUNT || !boardIsInside(x, y)) {
        return false;
    }
    unit = &game->units[unitIndex];
    if (!unit->alive) {
        return false;
    }
    /* 現在地を選ぶ「移動しない」行動も可 */
    if (x == unit->x && y == unit->y) {
        return true;
    }
    /* 目的地そのものが地形や他ユニットで塞がっていたら全種類共通で不可。 */
    if (!boardCanLand(game, unitIndex, x, y)) {
        return false;
    }

    /* 現在地との差分に直すと、盤面上の絶対位置に関係なく同じ式で判定できる。 */
    dx = x - unit->x;
    dy = y - unit->y;
    /* 三項演算子「条件 ? true側 : false側」。P1は上(-1)、P2は下(+1)が前。 */
    forward = unit->owner == PLAYER_ONE ? -1 : 1;

    switch (unit->type) {
        case UNIT_A:
            /* Aは向きに関係なく、上下左右へ1マス移動する。 */
            return abs(dx) + abs(dy) == 1;

        case UNIT_B:
            /*
             * Bは斜め四方へ1マス、または正面へ1マス移動する。
             * 斜め移動では横・縦の隣接マスを調べないため、そこが塞がってても
             * 斜めの着地点が空いていれば直接移動できる。
             */
            return (abs(dx) == 1 && abs(dy) == 1) ||
                   (dx == 0 && dy == forward);

        case UNIT_C:
            /* Cは前後左右へ1マス、または前方斜め2マスへ移動する。
             * 左前・右前も途中マスを調べず、着地点へ直接移動する。
             */
            return (dy == forward && abs(dx) <= 1) ||
                   (dy == 0 && abs(dx) == 1) ||
                   (dy == -forward && dx == 0);

        default:
            /* 壊れた種類値を移動可能にしない。 */
            return false;
    }
}

/* 敵の有無に関係なく、指定位置から対象マスへ攻撃が届くかを確認する。 */
bool boardCanAttackFrom(const Game *game, int attackerIndex, int fromX, int fromY,
                        int targetX, int targetY)
{
    const Unit *attacker;
    int dx;
    int dy;
    int forward;

    /* 配列番号と座標を検証してからunits[]やterrain[][]へアクセスする。 */
    if (attackerIndex < 0 || attackerIndex >= UNIT_COUNT ||
        !boardIsInside(fromX, fromY) || !boardIsInside(targetX, targetY)) {
        return false;
    }
    attacker = &game->units[attackerIndex];
    if (!attacker->alive) {
        return false;
    }
    dx = targetX - fromX;
    dy = targetY - fromY;
    forward = attacker->owner == PLAYER_ONE ? -1 : 1;

    switch (attacker->type) {
        case UNIT_A:
            /* Aは正面の1マス前または2マス前にいる敵1体を攻撃する。 */
            if (dx != 0 || (dy != forward && dy != forward * 2)) {
                return false;
            }
            if (dy == forward * 2) {
                int middleY = fromY + forward;
                int middleUnit = boardUnitAt(game, fromX, middleY);
                /* 2マス攻撃では、間のキャラや通行不可地形を貫通しない。 */
                if ((middleUnit >= 0 && middleUnit != attackerIndex) ||
                    !boardTerrainIsWalkable(game->terrain[middleY][fromX])) {
                    return false;
                }
            }
            return true;

        case UNIT_B:
            /*
             * Bは2マス前の左・中央・右にいる敵1体を攻撃する。
             * 1マス前の3マスは一切調べないため、キャラや地形を飛び越える。
             */
            return dy == forward * 2 && abs(dx) <= 1;

        case UNIT_C:
            /* Cは斜め四方の隣接マスを攻撃する。横・縦のマスは関係しない。 */
            return abs(dx) == 1 && abs(dy) == 1;

        default:
            return false;
    }
}

/* 攻撃者と対象が敵同士で、キャラ固有の攻撃範囲内かを確認する。 */
bool boardCanAttack(const Game *game, int attackerIndex, int targetIndex)
{
    const Unit *attacker;
    const Unit *target;

    /* 2つの配列番号を検証してからunits[]へアクセスする。 */
    if (attackerIndex < 0 || attackerIndex >= UNIT_COUNT ||
        targetIndex < 0 || targetIndex >= UNIT_COUNT) {
        return false;
    }
    attacker = &game->units[attackerIndex];
    target = &game->units[targetIndex];
    /* 死亡者同士や味方への攻撃は禁止。!は真偽を反転する演算子。 */
    if (!attacker->alive || !target->alive || attacker->owner == target->owner) {
        return false;
    }
    return boardCanAttackFrom(game, attackerIndex, attacker->x, attacker->y,
                              target->x, target->y);
}

/* 攻撃メニューを開いたとき、最初にカーソルを合わせる敵を探す。 */
int boardFindFirstAttackTarget(const Game *game, int attackerIndex)
{
    int i;

    for (i = 0; i < UNIT_COUNT; i++) {
        /* 既存のboardCanAttackを再利用し、判定式を二重管理しない。 */
        if (boardCanAttack(game, attackerIndex, i)) {
            return i;
        }
    }
    return -1;
}
