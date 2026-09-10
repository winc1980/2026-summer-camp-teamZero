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
 */

/* assert(条件)がfalseなら、その行でテストを失敗させる標準ヘッダ。 */
#include <assert.h>
/* putsで最終成功メッセージを表示する標準ヘッダ。 */
#include <stdio.h>
/* strcmpで画面表示用の固定名を確認するための標準ヘッダ。 */
#include <string.h>

#include "board.h"
#include "game.h"
#include "unit.h"

/* 何も押していない1フレーム分の入力を作る。 */
static GameInput noInput(void)
{
    /* {0}で構造体の全boolをfalseへする。 */
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
    assert(game.units[3].type == UNIT_A && game.units[3].x == 4 && game.units[3].y == 0);
    assert(game.units[4].type == UNIT_B && game.units[4].x == 3 && game.units[4].y == 0);
    assert(game.units[5].type == UNIT_C && game.units[5].x == 2 && game.units[5].y == 0);
}

/* 6体の陣営・タイプと、下画面へ出す名前の対応を確認する。 */
static void testCharacterNames(void)
{
    assert(strcmp(unitCharacterName(PLAYER_ONE, UNIT_A), "ゆうしゃ") == 0);
    assert(strcmp(unitCharacterName(PLAYER_ONE, UNIT_B), "まほうつかい") == 0);
    assert(strcmp(unitCharacterName(PLAYER_ONE, UNIT_C), "エルフ") == 0);
    assert(strcmp(unitCharacterName(PLAYER_TWO, UNIT_A), "フランケン") == 0);
    assert(strcmp(unitCharacterName(PLAYER_TWO, UNIT_B), "ゴースト") == 0);
    assert(strcmp(unitCharacterName(PLAYER_TWO, UNIT_C), "ヴァンパイア") == 0);
    assert(unitAttackForType(UNIT_A) == 40);
    assert(unitAttackForType(UNIT_B) == 30);
    assert(unitAttackForType(UNIT_C) == 30);
    assert(unitAwakenedAttackForType(UNIT_A) == 70);
    assert(unitAwakenedAttackForType(UNIT_B) == 40);
    assert(unitAwakenedAttackForType(UNIT_C) == 40);
    assert(strcmp(unitSkillName(PLAYER_ONE, UNIT_A, false), "ざんげき") == 0);
    assert(strcmp(unitSkillName(PLAYER_ONE, UNIT_A, true), "せいなるいちげき") == 0);
    assert(strcmp(unitSkillName(PLAYER_ONE, UNIT_B, false), "サンダースパイク") == 0);
    assert(strcmp(unitSkillName(PLAYER_ONE, UNIT_B, true), "サンダースパーク") == 0);
    assert(strcmp(unitSkillName(PLAYER_ONE, UNIT_C, false), "フェアリーアロー") == 0);
    assert(strcmp(unitSkillName(PLAYER_ONE, UNIT_C, true), "フェアリーブレス") == 0);
    assert(strcmp(unitSkillName(PLAYER_TWO, UNIT_A, false), "パワースマッシュ") == 0);
    assert(strcmp(unitSkillName(PLAYER_TWO, UNIT_A, true), "ボルトクラッシュ") == 0);
    assert(strcmp(unitSkillName(PLAYER_TWO, UNIT_B, false), "ポルターガイスト") == 0);
    assert(strcmp(unitSkillName(PLAYER_TWO, UNIT_B, true), "ナイトメアストーム") == 0);
    assert(strcmp(unitSkillName(PLAYER_TWO, UNIT_C, false), "ブラッドバイト") == 0);
    assert(strcmp(unitSkillName(PLAYER_TWO, UNIT_C, true), "ブラッドドレイン") == 0);
}

/* 盤面外表示にも使う、A・B・C本来の攻撃形状と陣営ごとの向きを確認する。 */
static void testAttackPatternOffsets(void)
{
    Game game;

    assert(boardIsAttackOffset(UNIT_A, PLAYER_ONE, 0, -1));
    assert(boardIsAttackOffset(UNIT_A, PLAYER_ONE, 0, -2));
    assert(!boardIsAttackOffset(UNIT_A, PLAYER_ONE, 0, 1));
    assert(boardIsAttackOffset(UNIT_A, PLAYER_TWO, 0, 2));
    assert(boardIsAttackOffset(UNIT_B, PLAYER_ONE, -1, -2));
    assert(boardIsAttackOffset(UNIT_B, PLAYER_ONE, 0, -2));
    assert(boardIsAttackOffset(UNIT_B, PLAYER_ONE, 1, -2));
    assert(!boardIsAttackOffset(UNIT_B, PLAYER_ONE, 0, -1));
    assert(boardIsAttackOffset(UNIT_C, PLAYER_ONE, -1, -1));
    assert(boardIsAttackOffset(UNIT_C, PLAYER_TWO, 1, 1));
    assert(!boardIsAttackOffset(UNIT_C, PLAYER_ONE, 0, -1));
    assert(!boardIsAttackOffset(UNIT_A, PLAYER_NONE, 0, -1));

    /* 攻撃形状として該当しても、実際の盤面外へは攻撃できない。 */
    gameInit(&game);
    game.units[0].x = 0;
    game.units[0].y = 1;
    assert(boardIsAttackOffset(UNIT_A, PLAYER_ONE, 0, -2));
    assert(!boardCanAttackFrom(&game, 0, 0, 1, 0, -1));
}

/* 障害物や盤面端に左右されない、A・B・C本来の移動形状を確認する。 */
static void testMovePatternOffsets(void)
{
    assert(boardIsMoveOffset(UNIT_A, PLAYER_ONE, 0, -1));
    assert(boardIsMoveOffset(UNIT_A, PLAYER_ONE, 1, 0));
    assert(!boardIsMoveOffset(UNIT_A, PLAYER_ONE, 1, -1));
    assert(boardIsMoveOffset(UNIT_B, PLAYER_ONE, -1, -1));
    assert(boardIsMoveOffset(UNIT_B, PLAYER_ONE, 0, -1));
    assert(!boardIsMoveOffset(UNIT_B, PLAYER_ONE, 0, 1));
    assert(boardIsMoveOffset(UNIT_B, PLAYER_TWO, 0, 1));
    assert(boardIsMoveOffset(UNIT_C, PLAYER_ONE, -1, -1));
    assert(boardIsMoveOffset(UNIT_C, PLAYER_ONE, 0, 1));
    assert(boardIsMoveOffset(UNIT_C, PLAYER_TWO, 1, 1));
    assert(!boardIsMoveOffset(UNIT_C, PLAYER_ONE, 1, 1));
    assert(!boardIsMoveOffset(UNIT_A, PLAYER_NONE, 0, -1));
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

/* 敵がいないマスにも、実際の攻撃と同じ範囲判定を再利用できることを確認。 */
static void testAttackablePositions(void)
{
    Game game;

    /* Aは正面1・2マス。2マス先は間の障害物で遮られる。 */
    gameInit(&game);
    game.units[0].x = 3;
    game.units[0].y = 4;
    assert(boardCanAttackFrom(&game, 0, 3, 4, 3, 3));
    assert(boardCanAttackFrom(&game, 0, 3, 4, 3, 2));
    assert(!boardCanAttackFrom(&game, 0, 3, 4, 2, 3));
    game.terrain[3][3] = TERRAIN_MOUNTAIN;
    assert(!boardCanAttackFrom(&game, 0, 3, 4, 3, 2));

    /* P2のAは向きが反転し、画面下方向の1・2マスへ届く。 */
    gameInit(&game);
    game.units[3].x = 3;
    game.units[3].y = 1;
    assert(boardCanAttackFrom(&game, 3, 3, 1, 3, 2));
    assert(boardCanAttackFrom(&game, 3, 3, 1, 3, 3));
    assert(!boardCanAttackFrom(&game, 3, 3, 1, 3, 0));

    /* P2のBは画面下方向2マス先の横3マス。 */
    gameInit(&game);
    game.units[4].x = 3;
    game.units[4].y = 1;
    assert(boardCanAttackFrom(&game, 4, 3, 1, 2, 3));
    assert(boardCanAttackFrom(&game, 4, 3, 1, 3, 3));
    assert(boardCanAttackFrom(&game, 4, 3, 1, 4, 3));
    assert(!boardCanAttackFrom(&game, 4, 3, 1, 3, 2));

    /* Cは斜め四方だけ。 */
    gameInit(&game);
    game.units[2].x = 3;
    game.units[2].y = 3;
    assert(boardCanAttackFrom(&game, 2, 3, 3, 2, 2));
    assert(boardCanAttackFrom(&game, 2, 3, 3, 4, 2));
    assert(boardCanAttackFrom(&game, 2, 3, 3, 2, 4));
    assert(boardCanAttackFrom(&game, 2, 3, 3, 4, 4));
    assert(!boardCanAttackFrom(&game, 2, 3, 3, 3, 2));

    /* 実際の座標を書き換えなくても、仮の移動先から範囲を計算できる。 */
    assert(boardCanAttackFrom(&game, 2, 4, 3, 5, 2));

    /* 不正な番号・盤外座標は配列へ触らず拒否する。 */
    assert(!boardCanAttackFrom(&game, -1, 0, 0, 0, 0));
    assert(!boardCanAttackFrom(&game, UNIT_COUNT, 0, 0, 0, 0));
    assert(!boardCanAttackFrom(&game, 2, -1, 0, 0, 0));
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
    game.units[3].hp = 40;
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
    assert(game.cursorX == 4 && game.cursorY == 0);
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

/* 味方を1体失った側が、次の手番で生存者から覚醒対象を選べることを確認する。 */
static void testAwakeningSelection(void)
{
    Game game;
    gameInit(&game);
    game.units[5].alive = false;
    game.units[5].hp = 0;

    makeFocusedUnitWait(&game);
    makeFocusedUnitWait(&game);
    makeFocusedUnitWait(&game);
    assert(game.currentPlayer == PLAYER_TWO);
    assert(game.phase == PHASE_SELECT_AWAKENING);
    assert(game.cursorX == game.units[3].x && game.cursorY == game.units[3].y);

    gameUpdate(&game, downInput());
    assert(game.cursorX == game.units[4].x && game.cursorY == game.units[4].y);
    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_UNIT);
    assert(game.awakeningChosen[PLAYER_TWO]);
    assert(game.units[4].awakened);
    assert(game.units[4].attack == 40);
    assert(!game.units[4].acted);
    assert(!game.units[3].awakened);
}

/* 生存者が1体だけなら選択を挟まず、その1体が自動覚醒することを確認する。 */
static void testAutomaticAwakening(void)
{
    Game game;
    gameInit(&game);
    game.units[3].alive = false;
    game.units[3].hp = 0;
    game.units[4].alive = false;
    game.units[4].hp = 0;

    makeFocusedUnitWait(&game);
    makeFocusedUnitWait(&game);
    makeFocusedUnitWait(&game);
    assert(game.currentPlayer == PLAYER_TWO);
    assert(game.phase == PHASE_SELECT_UNIT);
    assert(game.awakeningChosen[PLAYER_TWO]);
    assert(game.units[5].awakened);
    assert(game.units[5].attack == 40);
}

/* 覚醒Aは正面2マスの敵を貫通してまとめて攻撃する。 */
static void testAwakenedAAreaAttack(void)
{
    Game game;
    gameInit(&game);
    game.units[0].x = 3;
    game.units[0].y = 4;
    game.units[0].awakened = true;
    game.units[0].attack = unitAwakenedAttackForType(UNIT_A);
    game.units[3].x = 3;
    game.units[3].y = 3;
    game.units[4].x = 3;
    game.units[4].y = 2;
    game.units[5].x = 7;
    game.units[5].y = 0;
    game.selectedUnit = 0;
    game.selectedAction = ACTION_ATTACK;
    game.phase = PHASE_SELECT_ACTION;

    assert(boardCanAttack(&game, 0, 3));
    assert(boardCanAttack(&game, 0, 4));
    gameUpdate(&game, confirmInput());
    assert(game.units[3].hp == 30);
    assert(game.units[4].hp == 30);
    assert(game.units[0].acted);
    assert(game.phase == PHASE_SELECT_UNIT);
}

/* 覚醒Bは2マス前の横3マスにいる敵を一度に攻撃する。 */
static void testAwakenedBAreaAttack(void)
{
    Game game;
    gameInit(&game);
    game.units[1].x = 3;
    game.units[1].y = 4;
    game.units[1].awakened = true;
    game.units[1].attack = unitAwakenedAttackForType(UNIT_B);
    game.units[3].x = 2;
    game.units[3].y = 2;
    game.units[4].x = 4;
    game.units[4].y = 2;
    game.units[5].x = 7;
    game.units[5].y = 0;
    game.selectedUnit = 1;
    game.selectedAction = ACTION_ATTACK;
    game.phase = PHASE_SELECT_ACTION;

    gameUpdate(&game, confirmInput());
    assert(game.units[3].hp == 60);
    assert(game.units[4].hp == 60);
    assert(game.units[1].acted);
}

/* 範囲攻撃で残る敵を同時に倒した場合も、その場で勝敗が決まる。 */
static void testAreaAttackCanWin(void)
{
    Game game;
    gameInit(&game);
    game.units[1].x = 3;
    game.units[1].y = 4;
    game.units[1].awakened = true;
    game.units[1].attack = unitAwakenedAttackForType(UNIT_B);
    game.units[3].x = 2;
    game.units[3].y = 2;
    game.units[3].hp = 40;
    game.units[4].x = 4;
    game.units[4].y = 2;
    game.units[4].hp = 40;
    game.units[5].alive = false;
    game.units[5].hp = 0;
    game.selectedUnit = 1;
    game.selectedAction = ACTION_ATTACK;
    game.phase = PHASE_SELECT_ACTION;

    gameUpdate(&game, confirmInput());
    assert(!game.units[3].alive && !game.units[4].alive);
    assert(game.phase == PHASE_GAME_OVER);
    assert(game.winner == PLAYER_ONE);
}

/* 覚醒Cは単体攻撃の命中時に、自身を20回復し最大HPを越えない。 */
static void testAwakenedCHealing(void)
{
    Game game;
    gameInit(&game);
    game.units[2].x = 3;
    game.units[2].y = 3;
    game.units[2].hp = 85;
    game.units[2].awakened = true;
    game.units[2].attack = unitAwakenedAttackForType(UNIT_C);
    game.units[3].x = 2;
    game.units[3].y = 2;
    game.selectedUnit = 2;
    game.selectedAction = ACTION_ATTACK;
    game.phase = PHASE_SELECT_ACTION;

    gameUpdate(&game, confirmInput());
    assert(game.phase == PHASE_SELECT_TARGET);
    gameUpdate(&game, confirmInput());
    assert(game.units[3].hp == 60);
    assert(game.units[2].hp == INITIAL_HP);
}

/* 再戦時は、両者の覚醒履歴と全キャラクターの能力が初期値へ戻る。 */
static void testAwakeningResetsOnReplay(void)
{
    Game game;
    GameInput restart = noInput();
    int i;
    gameInit(&game);
    game.units[0].awakened = true;
    game.units[0].attack = unitAwakenedAttackForType(UNIT_A);
    game.awakeningChosen[PLAYER_ONE] = true;
    game.phase = PHASE_GAME_OVER;
    restart.restart = true;
    gameUpdate(&game, restart);

    assert(!game.awakeningChosen[PLAYER_ONE]);
    assert(!game.awakeningChosen[PLAYER_TWO]);
    for (i = 0; i < UNIT_COUNT; i++) {
        assert(!game.units[i].awakened);
        assert(game.units[i].attack == unitAttackForType(game.units[i].type));
    }
}

/* 通常プログラムと同じくテスト実行ファイルもmainから開始する。 */
int main(void)
{
    /* 途中のassertが1つでも失敗すれば、その場で終了して問題行を表示する。 */
    testInitialState();
    testCharacterNames();
    testAttackPatternOffsets();
    testMovePatternOffsets();
    testMovementRules();
    testAttackRanges();
    testAttackablePositions();
    testCancelRestoresPosition();
    testAttackAndVictory();
    testTurnChangesAfterAllUnitsAct();
    testActionMenuDoesNotMoveBoardCursor();
    testUnavailableAttackSelectsWaitOnly();
    testAwakeningSelection();
    testAutomaticAwakening();
    testAwakenedAAreaAttack();
    testAwakenedBAreaAttack();
    testAreaAttackCanWin();
    testAwakenedCHealing();
    testAwakeningResetsOnReplay();
    /* ここまで到達した場合だけ、すべて成功したと表示する。 */
    puts("All game rule tests passed.");
    return 0;
}
