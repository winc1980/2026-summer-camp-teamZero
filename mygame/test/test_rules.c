/*
 * test_rules.c — DSを起動せずゲームルールを確認するテスト
 * --------------------------------------------------------------------------
 * macOS/Linuxの通常のCコンパイラでgame.c・board.c・unit.cを実行し、
 * 初期配置、移動、攻撃、キャンセル、ターン交代などをassertで確認する。
 * 描画と入力ハードウェアをルールから分離しているので、同じコードをPC上で
 * 素早く検証できる。
 *
 * ルールを変更するときは、実装だけでなく対応するテストも更新すること。
 * 既存仕様を意図せず壊していないか確認するため、PR前には必ず実行する。
 * 実行コマンドはmygame/readme.mdに記載してある。
 *
 * 参考資料:
 *  事前資料 3章: テストを目的別の関数へ分割する
 *  事前資料 4章: Game *を更新関数へ渡す
 *  事前資料 6章: テスト用のGameInput構造体を作る
 *  事前資料 11章: assertで期待値を自動確認する
 *  ユニットテストの運用は、事前資料からネットの参考に発展させた内容
 */

/* assert(条件)がfalseなら、その行でテストを失敗させる標準ヘッダ。 */
#include <assert.h>
/* putsで最終成功メッセージを表示する標準ヘッダ。 */
#include <stdio.h>

#include "board.h"
#include "game.h"

/* 何も押していない1フレーム分の入力を作る。 */
static GameInput noInput(void)
{
    /* {0}で構造体の全boolをfalseへする（6章）。 */
    GameInput input = {0};
    return input;
}

/* noInputを土台に、A（決定）だけtrueにする。 */
static GameInput confirmInput(void)
{
    GameInput input = noInput();
    input.confirm = true;
    return input;
}

/* B（取消）だけを押した入力。 */
static GameInput cancelInput(void)
{
    GameInput input = noInput();
    input.cancel = true;
    return input;
}

/* 下方向だけを押した入力。行動メニュー切替テストで使う。 */
static GameInput downInput(void)
{
    GameInput input = noInput();
    input.down = true;
    return input;
}

/* gameInit直後の手番・phase・6体の種類と配置を確認する。 */
static void testInitialState(void)
{
    Game game;
    gameInit(&game);

    /* assert内が全てtrueなら何も出さず次へ進む。 */
    assert(game.currentPlayer == PLAYER_ONE);
    assert(game.phase == PHASE_SELECT_UNIT);
    assert(game.units[0].type == UNIT_A && game.units[0].x == 2 && game.units[0].y == 5);
    assert(game.units[1].type == UNIT_B && game.units[1].x == 3 && game.units[1].y == 5);
    assert(game.units[2].type == UNIT_C && game.units[2].x == 4 && game.units[2].y == 5);
    assert(game.units[3].type == UNIT_A && game.units[3].x == 2 && game.units[3].y == 0);
    assert(game.units[5].type == UNIT_C && game.units[5].x == 4 && game.units[5].y == 0);
}

/* A/B/Cの移動方向、斜め移動、障害物、着地点ルールを直接確認する。 */
static void testMovementRules(void)
{
    Game game;

    /* A: 上下左右1マスは可能。斜めや2マス移動は不可。 */
    gameInit(&game);
    game.units[0].x = 3;
    game.units[0].y = 3;
    assert(boardCanMoveTo(&game, 0, 3, 2));
    assert(boardCanMoveTo(&game, 0, 3, 4));
    assert(boardCanMoveTo(&game, 0, 2, 3));
    assert(boardCanMoveTo(&game, 0, 4, 3));
    assert(!boardCanMoveTo(&game, 0, 2, 2));
    assert(!boardCanMoveTo(&game, 0, 3, 1));

    /* B: 斜め四方1マスと正面1マスが可能。横・後ろ・前2は不可。 */
    gameInit(&game);
    game.units[1].x = 3;
    game.units[1].y = 3;
    assert(boardCanMoveTo(&game, 1, 2, 2));
    assert(boardCanMoveTo(&game, 1, 4, 2));
    assert(boardCanMoveTo(&game, 1, 2, 4));
    assert(boardCanMoveTo(&game, 1, 4, 4));
    assert(boardCanMoveTo(&game, 1, 3, 2));
    assert(!boardCanMoveTo(&game, 1, 2, 3));
    assert(!boardCanMoveTo(&game, 1, 3, 4));
    assert(!boardCanMoveTo(&game, 1, 3, 1));
    /* 横と縦の隣接マスが障害物でも、空いている斜めへ直接移動できる。 */
    game.terrain[2][3] = TERRAIN_MOUNTAIN;
    game.terrain[3][2] = TERRAIN_MOUNTAIN;
    assert(boardCanMoveTo(&game, 1, 2, 2));
    game.terrain[2][3] = TERRAIN_PLAIN;
    game.terrain[3][2] = TERRAIN_PLAIN;
    game.units[0].x = 3;
    game.units[0].y = 2;
    game.units[2].x = 2;
    game.units[2].y = 3;
    assert(boardCanMoveTo(&game, 1, 2, 2));
    /* 斜めの着地点そのものが障害物なら移動できない。 */
    game.terrain[2][2] = TERRAIN_MOUNTAIN;
    assert(!boardCanMoveTo(&game, 1, 2, 2));

    /* C: 前方1列の3マス、左右、真後ろへ1マス移動できる。 */
    gameInit(&game);
    game.units[2].x = 3;
    game.units[2].y = 3;
    assert(boardCanMoveTo(&game, 2, 2, 2));
    assert(boardCanMoveTo(&game, 2, 3, 2));
    assert(boardCanMoveTo(&game, 2, 4, 2));
    assert(boardCanMoveTo(&game, 2, 2, 3));
    assert(boardCanMoveTo(&game, 2, 4, 3));
    assert(boardCanMoveTo(&game, 2, 3, 4));
    assert(!boardCanMoveTo(&game, 2, 2, 4));
    assert(!boardCanMoveTo(&game, 2, 3, 1));
    /* 正面と横が障害物でも、空いている左前へ直接移動できる。 */
    game.terrain[2][3] = TERRAIN_MOUNTAIN;
    game.terrain[3][2] = TERRAIN_MOUNTAIN;
    assert(boardCanMoveTo(&game, 2, 2, 2));
    game.terrain[2][3] = TERRAIN_PLAIN;
    game.terrain[3][2] = TERRAIN_PLAIN;
    game.units[0].x = 3;
    game.units[0].y = 2;
    game.units[1].x = 2;
    game.units[1].y = 3;
    assert(boardCanMoveTo(&game, 2, 2, 2));

    /* P2では前方向が画面下になる。 */
    gameInit(&game);
    game.units[4].x = 3;
    game.units[4].y = 3;
    assert(boardCanMoveTo(&game, 4, 3, 4));
    assert(!boardCanMoveTo(&game, 4, 3, 2));
    game.units[5].x = 3;
    game.units[5].y = 3;
    assert(boardCanMoveTo(&game, 5, 2, 4));
    assert(boardCanMoveTo(&game, 5, 3, 2));
}

/* 攻撃範囲テスト用に、攻撃者と対象以外を一度盤面から除く。 */
static void keepOnlyUnits(Game *game, int attackerIndex, int targetIndex)
{
    int i;
    for (i = 0; i < UNIT_COUNT; i++) game->units[i].alive = false;
    game->units[attackerIndex].alive = true;
    game->units[targetIndex].alive = true;
}

/* A/B/C固有の攻撃範囲と、飛び越え可否を確認する。 */
static void testAttackRanges(void)
{
    Game game;

    /* A: 正面1・2マスを攻撃できるが、横と3マス前は不可。 */
    gameInit(&game);
    keepOnlyUnits(&game, 0, 3);
    game.units[0].x = 3;
    game.units[0].y = 4;
    game.units[3].x = 3;
    game.units[3].y = 3;
    assert(boardCanAttack(&game, 0, 3));
    game.units[3].y = 2;
    assert(boardCanAttack(&game, 0, 3));
    game.units[3].y = 1;
    assert(!boardCanAttack(&game, 0, 3));
    game.units[3].x = 2;
    game.units[3].y = 3;
    assert(!boardCanAttack(&game, 0, 3));
    /* 2マス前への攻撃は、間にキャラまたは障害物があれば届かない。 */
    game.units[3].x = 3;
    game.units[3].y = 2;
    game.units[1].alive = true;
    game.units[1].x = 3;
    game.units[1].y = 3;
    assert(!boardCanAttack(&game, 0, 3));
    game.units[1].alive = false;
    game.terrain[3][3] = TERRAIN_MOUNTAIN;
    assert(!boardCanAttack(&game, 0, 3));

    /* B: 2マス前の横3マスを攻撃できる。1マス前は不可。 */
    gameInit(&game);
    keepOnlyUnits(&game, 1, 3);
    game.units[1].x = 3;
    game.units[1].y = 4;
    game.units[3].y = 2;
    game.units[3].x = 2;
    assert(boardCanAttack(&game, 1, 3));
    game.units[3].x = 3;
    assert(boardCanAttack(&game, 1, 3));
    game.units[3].x = 4;
    assert(boardCanAttack(&game, 1, 3));
    game.units[3].x = 1;
    assert(!boardCanAttack(&game, 1, 3));
    game.units[3].x = 3;
    game.units[3].y = 3;
    assert(!boardCanAttack(&game, 1, 3));
    /* 1マス前の横3マスが障害物でも、2マス前へ飛び越えて攻撃できる。 */
    game.units[3].y = 2;
    game.terrain[3][2] = TERRAIN_MOUNTAIN;
    game.terrain[3][3] = TERRAIN_MOUNTAIN;
    game.terrain[3][4] = TERRAIN_MOUNTAIN;
    assert(boardCanAttack(&game, 1, 3));
    game.units[0].alive = true;
    game.units[0].x = 2;
    game.units[0].y = 3;
    game.units[2].alive = true;
    game.units[2].x = 3;
    game.units[2].y = 3;
    game.units[4].alive = true;
    game.units[4].x = 4;
    game.units[4].y = 3;
    assert(boardCanAttack(&game, 1, 3));

    /* C: 斜め四方1マスだけを攻撃する。 */
    gameInit(&game);
    keepOnlyUnits(&game, 2, 3);
    game.units[2].x = 3;
    game.units[2].y = 3;
    game.units[3].x = 2;
    game.units[3].y = 2;
    assert(boardCanAttack(&game, 2, 3));
    game.units[3].x = 4;
    assert(boardCanAttack(&game, 2, 3));
    game.units[3].y = 4;
    assert(boardCanAttack(&game, 2, 3));
    game.units[3].x = 2;
    assert(boardCanAttack(&game, 2, 3));
    game.units[3].x = 3;
    assert(!boardCanAttack(&game, 2, 3));
    /* 横と縦のマスが障害物でも、斜めの対象には攻撃できる。 */
    game.units[3].x = 2;
    game.units[3].y = 2;
    game.terrain[2][3] = TERRAIN_MOUNTAIN;
    game.terrain[3][2] = TERRAIN_MOUNTAIN;
    assert(boardCanAttack(&game, 2, 3));

    /* P2の正面は画面下。Bは下2マスを攻撃する。 */
    gameInit(&game);
    keepOnlyUnits(&game, 4, 0);
    game.units[4].x = 3;
    game.units[4].y = 1;
    game.units[0].x = 3;
    game.units[0].y = 3;
    assert(boardCanAttack(&game, 4, 0));
}

/* 行動選択中にBを押すと、仮移動前の座標へ戻ることを確認する。 */
static void testCancelRestoresPosition(void)
{
    Game game;
    gameInit(&game);

    /* Aを選択→1マス前へ仮移動→B取消、という実際の入力順を再現する。 */
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_MOVE);
    game.cursorY = 4;
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_ACTION);
    assert(game.units[0].y == 4);
    gameUpdate(&game, cancelInput());
    assert(game.phase == PHASE_SELECT_MOVE);
    assert(game.units[0].x == 2 && game.units[0].y == 5);
}

/* 最後の敵を倒す攻撃、勝敗、START再戦を一連の状態遷移として確認する。 */
static void testAttackAndVictory(void)
{
    Game game;
    gameInit(&game);

    /* テストを短くするため、敵Aを隣へ置き、残り2体を倒れた状態にする。 */
    game.units[3].x = 2;
    game.units[3].y = 4;
    game.units[3].hp = 60;
    game.units[4].alive = false;
    game.units[5].alive = false;

    /* 選択→その場移動→攻撃選択→対象決定の順にA入力を送る。 */
    gameUpdate(&game, confirmInput());
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_ACTION);
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_TARGET);
    assert(game.cursorX == 2 && game.cursorY == 4);
    gameUpdate(&game, confirmInput());
    assert(!game.units[3].alive);
    assert(game.units[3].hp == 0);
    assert(game.phase == PHASE_GAME_OVER);
    assert(game.winner == PLAYER_ONE);

    /* 波括弧でrestart変数の有効範囲（スコープ）をこの部分だけに限定する。 */
    {
        GameInput restart = noInput();
        restart.restart = true;
        gameUpdate(&game, restart);
    }
    assert(game.phase == PHASE_SELECT_UNIT);
    assert(game.winner == PLAYER_NONE);
}

/* 現在カーソルが合っているユニットを、その場WAITまで進める共通手順。 */
static void makeFocusedUnitWait(Game *game)
{
    gameUpdate(game, confirmInput());
    assert(game->phase == PHASE_SELECT_MOVE);
    gameUpdate(game, confirmInput());
    assert(game->phase == PHASE_SELECT_ACTION);
    gameUpdate(game, downInput());
    assert(game->selectedAction == ACTION_WAIT);
    gameUpdate(game, confirmInput());
}

/* 生存3体全員がWAITした時点でP2へターン交代することを確認する */
static void testTurnChangesAfterAllUnitsAct(void)
{
    Game game;
    gameInit(&game);

    makeFocusedUnitWait(&game);
    assert(game.currentPlayer == PLAYER_ONE);
    makeFocusedUnitWait(&game);
    assert(game.currentPlayer == PLAYER_ONE);
    makeFocusedUnitWait(&game);
    assert(game.currentPlayer == PLAYER_TWO);
    assert(game.phase == PHASE_SELECT_UNIT);
    assert(game.cursorX == 2 && game.cursorY == 0);
}

/* 行動メニューの方向入力が、黄色い盤面カーソルを動かさないことを確認する。 */
static void testActionMenuDoesNotMoveBoardCursor(void)
{
    Game game;
    int cursorX;
    int cursorY;
    gameInit(&game);

    gameUpdate(&game, confirmInput());
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_ACTION);
    cursorX = game.cursorX;
    cursorY = game.cursorY;
    gameUpdate(&game, downInput());
    assert(game.selectedAction == ACTION_WAIT);
    assert(game.cursorX == cursorX && game.cursorY == cursorY);
}

/* 攻撃対象の有無に応じ、行動メニューで選べる項目が変わることを確認する。 */
static void testUnavailableAttackSelectsWaitOnly(void)
{
    Game game;
    gameInit(&game);

    /* 初期位置のAには攻撃対象がいないので、移動しない確定後はWAITになる。 */
    gameUpdate(&game, confirmInput());
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_ACTION);
    assert(game.selectedAction == ACTION_WAIT);

    /* 方向キーを押しても、選べないATTACKには移動しない。 */
    gameUpdate(&game, downInput());
    assert(game.selectedAction == ACTION_WAIT);
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_UNIT);
    assert(game.units[0].acted);

    /* 攻撃範囲内に敵がいれば、従来どおりATTACKとWAITを切り替えられる。 */
    gameInit(&game);
    game.units[3].x = 2;
    game.units[3].y = 4;
    gameUpdate(&game, confirmInput());
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_ACTION);
    assert(game.selectedAction == ACTION_ATTACK);
    gameUpdate(&game, downInput());
    assert(game.selectedAction == ACTION_WAIT);
    gameUpdate(&game, downInput());
    assert(game.selectedAction == ACTION_ATTACK);
}

/* 通常プログラムと同じくテスト実行ファイルもmainから開始する。 */
int main(void)
{
    /* 途中のassertが1つでも失敗すれば、その場で終了して問題行を表示する。 */
    testInitialState();
    testMovementRules();
    testAttackRanges();
    testCancelRestoresPosition();
    testAttackAndVictory();
    testTurnChangesAfterAllUnitsAct();
    testActionMenuDoesNotMoveBoardCursor();
    testUnavailableAttackSelectsWaitOnly();
    /* ここまで到達した場合だけ、すべて成功したと表示する。 */
    puts("All game rule tests passed.");
    return 0;
}
