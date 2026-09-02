/*
 * game.c — ターン進行とゲーム状態の更新
 * --------------------------------------------------------------------------
 * キャラクター選択、移動先選択、行動選択、攻撃対象選択、ターン交代、
 * ダメージ、勝敗判定を担当します。1フレーム分の入力はgameUpdate()へ渡され、
 * 現在のGamePhaseに応じた処理だけが実行されます。
 *
 * ファイル間の分担:
 * - game.c  : 「いつ、何を起こすか」を決める
 * - board.c : 「その移動・攻撃が可能か」を判定する
 * - unit.c  : キャラクター1体の初期値を決める
 * - render.c: 更新後の状態を表示する（game.cからは呼ばない）
 *
 * 処理の流れは、おおむね次の順です。
 * PHASE_SELECT_UNIT → PHASE_SELECT_MOVE → PHASE_SELECT_ACTION
 * → 必要ならPHASE_SELECT_TARGET → そのキャラの行動終了
 * 自軍の生存キャラが全員行動すると、相手のターンへ移ります。
 *
 * 変更時の目安:
 * - 行動順、キャンセル、ターン交代、勝利条件はこのファイルを変更する
 * - A/B/Cの移動・攻撃範囲はboard.cを変更する
 * - 攻撃力や初期HPはunit.c / game_types.hを変更する
 * - phaseを追加した場合はgameUpdate()と表示側render.cの両方を確認する
 *
 * 参考資料:
 * - 事前資料 3章: static関数、引数、戻り値による処理分割
 * - 事前資料 4章: Game *を通して同じ試合状態を更新する
 * - 事前資料 5章: units[]、message[]、安全な文字列操作
 * - 事前資料 6章: Game、Unit、GameInput構造体
 * - 事前資料 10章: update関数、enum + switchによる状態遷移
 * - 事前資料 11章: 不正な値を早めに拒否する防御的な判定
 * - 可変長引数と有限状態機械の具体的な組み方は本作で追加した内容
 */

/* va_list等、引数の個数が変わる関数を作るための標準ヘッダ（資料外）。 */
#include <stdarg.h>
/* vsnprintfを使うための標準入出力ヘッダ。 */
#include <stdio.h>
/* memsetを使うための文字列・メモリ操作ヘッダ（5章）。 */
#include <string.h>

#include "board.h"
#include "game.h"
#include "unit.h"

/* P1ならP2、P2ならP1を返す小さな補助関数。 */
static Player otherPlayer(Player player)
{
    /* 三項演算子でif/elseを1つの式として書いている（2章）。 */
    return player == PLAYER_ONE ? PLAYER_TWO : PLAYER_ONE;
}

/*
 * printfと同様に%d等を使って下画面メッセージを組み立てる。
 * ... は可変長引数といい、事前資料外のC文法。sizeofで配列容量を渡すため、
 * 長すぎる文字列でもmessage[96]を越えて書かない（5章のsnprintfと同じ考え）。
 */
static void gameSetMessage(Game *game, const char *format, ...)
{
    va_list args;
    /* va_startからva_endまでが追加引数を読み取れる期間。 */
    va_start(args, format);
    /* sizeof(game->message)は96。末尾'\0'を含めて安全に打ち切る。 */
    vsnprintf(game->message, sizeof(game->message), format, args);
    va_end(args);
}

/* 指定プレイヤーにalive==trueのユニットが1体でもいるか調べる。 */
static bool gamePlayerHasLivingUnits(const Game *game, Player player)
{
    int i;
    for (i = 0; i < UNIT_COUNT; i++) {
        if (game->units[i].alive && game->units[i].owner == player) {
            /* 1体見つかった時点で残りを調べずtrueを返す早期return。 */
            return true;
        }
    }
    return false;
}

/* 生存している自軍全員が行動済みならtrue。死亡者は数えない。 */
static bool gameAllLivingUnitsActed(const Game *game, Player player)
{
    int i;
    for (i = 0; i < UNIT_COUNT; i++) {
        const Unit *unit = &game->units[i];
        /* 1体でも未行動がいれば「全員行動済み」ではない。 */
        if (unit->alive && unit->owner == player && !unit->acted) {
            return false;
        }
    }
    return true;
}

/* 新しい自分ターンの開始時、指定側のactedをすべてfalseへ戻す。 */
static void gameResetActed(Game *game, Player player)
{
    int i;
    for (i = 0; i < UNIT_COUNT; i++) {
        if (game->units[i].owner == player) {
            game->units[i].acted = false;
        }
    }
}

/* 次に選べる最初の自軍ユニットへ黄色カーソルを合わせる。 */
static void gameFocusFirstAvailableUnit(Game *game)
{
    int i;
    for (i = 0; i < UNIT_COUNT; i++) {
        const Unit *unit = &game->units[i];
        if (unit->alive && unit->owner == game->currentPlayer && !unit->acted) {
            game->cursorX = unit->x;
            game->cursorY = unit->y;
            /* 座標をコピーしたら目的達成なのでループと関数を終了する。 */
            return;
        }
    }
}

/* 相手へ手番を渡し、そのプレイヤーの行動状態と画面状態を初期化する。 */
static void gameBeginNextTurn(Game *game)
{
    /* 現在プレイヤーを反対側へ置き換える。 */
    game->currentPlayer = otherPlayer(game->currentPlayer);
    gameResetActed(game, game->currentPlayer);
    game->selectedUnit = -1;
    game->phase = PHASE_SELECT_UNIT;
    gameFocusFirstAvailableUnit(game);
    gameSetMessage(game, "プレイヤー%dのばん", (int)game->currentPlayer + 1);
}

/* 1体のATTACKまたはWAITが確定した後の共通処理。 */
static void gameFinishAction(Game *game)
{
    Player enemy = otherPlayer(game->currentPlayer);

    /* selectedUnitは選択中の配列番号。現在ターンでは再選択できなくする。 */
    game->units[game->selectedUnit].acted = true;
    /* 相手の生存者が0なら、ターン交代せずゲーム終了状態へ進む。 */
    if (!gamePlayerHasLivingUnits(game, enemy)) {
        game->winner = game->currentPlayer;
        game->phase = PHASE_GAME_OVER;
        gameSetMessage(game, "プレイヤー%dのかち START:もういちど", (int)game->winner + 1);
        return;
    }

    game->selectedUnit = -1;
    game->phase = PHASE_SELECT_UNIT;
    /* 自軍の生存者が全員行動したときだけ相手ターンへ進む。 */
    if (gameAllLivingUnitsActed(game, game->currentPlayer)) {
        gameBeginNextTurn(game);
    } else {
        gameFocusFirstAvailableUnit(game);
        gameSetMessage(game, "ほかのキャラをえらぶ");
    }
}

/* 十字キー入力から「次のカーソル候補」を計算し、盤内なら確定する。 */
static void gameMoveCursor(Game *game, GameInput input)
{
    int nextX = game->cursorX;
    int nextY = game->cursorY;

    /* else ifなので、同じフレームに複数方向が来ても1方向だけ動く。 */
    if (input.left) nextX--;
    else if (input.right) nextX++;
    else if (input.up) nextY--;
    else if (input.down) nextY++;

    /* 盤外なら代入しないため、カーソルは端で止まる。 */
    if (boardIsInside(nextX, nextY)) {
        game->cursorX = nextX;
        game->cursorY = nextY;
    }
}

/* PHASE_SELECT_UNIT中のAボタン処理。 */
static void gameUpdateSelectUnit(Game *game, GameInput input)
{
    int index;

    /* Aが押されていないフレームは何も変更しない。 */
    if (!input.confirm) return;
    /* カーソル位置のユニット番号を取得。空きマスなら-1。 */
    index = boardUnitAt(game, game->cursorX, game->cursorY);
    if (index < 0 || game->units[index].owner != game->currentPlayer) {
        gameSetMessage(game, "じぶんのキャラをえらぶ");
        return;
    }
    if (game->units[index].acted) {
        gameSetMessage(game, "このキャラはこうどうずみ");
        return;
    }

    /*
     * 仮移動をBで取り消せるよう、選択時点の座標をoriginへ保存する。
     * selectedUnitにはポインタではなく配列番号を持たせ、状態保存を簡単にする。
     */
    game->selectedUnit = index;
    game->originX = game->units[index].x;
    game->originY = game->units[index].y;
    game->phase = PHASE_SELECT_MOVE;
    gameSetMessage(game, "いどうさきをえらぶ B:もどる");
}

/* PHASE_SELECT_MOVE中の決定・取消処理。 */
static void gameUpdateSelectMove(Game *game, GameInput input)
{
    /* 選択中番号から、実際に座標を書き換えるUnitへのポインタを得る。 */
    Unit *unit = &game->units[game->selectedUnit];
    int target;

    /* Bならユニット選択へ戻る。まだ移動前なので座標復元は不要。 */
    if (input.cancel) {
        game->selectedUnit = -1;
        game->phase = PHASE_SELECT_UNIT;
        gameSetMessage(game, "キャラをえらぶ");
        return;
    }
    if (!input.confirm) return;
    /* board.cの共通ルールで、目的地が合法か最終確認する。 */
    if (!boardCanMoveTo(game, game->selectedUnit, game->cursorX, game->cursorY)) {
        gameSetMessage(game, "そこにはいどうできない");
        return;
    }

    /*
     * ここでは移動を一旦反映するが、行動選択中のBでoriginへ戻せる。
     * この方式を「仮適用して後からロールバックする」と考えられる（資料外）。
     */
    unit->x = game->cursorX;
    unit->y = game->cursorY;
    game->phase = PHASE_SELECT_ACTION;
    /*
     * 移動後の位置から攻撃できる敵を調べる。
     * 対象がいなければ最初からWAITを選び、選べないATTACKを押す手間をなくす。
     */
    target = boardFindFirstAttackTarget(game, game->selectedUnit);
    if (target >= 0) {
        game->selectedAction = ACTION_ATTACK;
        gameSetMessage(game, "こうげき または たいき");
    } else {
        game->selectedAction = ACTION_WAIT;
        gameSetMessage(game, "こうげきできない たいきをえらぶ");
    }
}

/* PHASE_SELECT_ACTION中の「こうげき／たいき」メニュー処理。 */
static void gameUpdateSelectAction(Game *game, GameInput input)
{
    Unit *unit = &game->units[game->selectedUnit];
    int target = boardFindFirstAttackTarget(game, game->selectedUnit);

    /* 攻撃対象がいなければWAITへ固定し、方向キーでもATTACKへ移動しない。 */
    if (target < 0) {
        game->selectedAction = ACTION_WAIT;
    }
    /* 攻撃可能なときだけ、方向キーでATTACKとWAITを交互に切り替える。 */
    if (target >= 0 && (input.up || input.down || input.left || input.right)) {
        game->selectedAction = game->selectedAction == ACTION_ATTACK
            ? ACTION_WAIT : ACTION_ATTACK;
    }
    /* Bなら仮移動を元座標へ戻し、移動先選択へ戻る。 */
    if (input.cancel) {
        unit->x = game->originX;
        unit->y = game->originY;
        game->cursorX = game->originX;
        game->cursorY = game->originY;
        game->phase = PHASE_SELECT_MOVE;
        gameSetMessage(game, "いどうさきをえらぶ B:もどる");
        return;
    }
    if (!input.confirm) return;

    /* WAITは対象選択がないため、その場で1体の行動を終了する。 */
    if (game->selectedAction == ACTION_WAIT) {
        gameFinishAction(game);
        return;
    }

    /* ATTACKはtarget>=0のときだけ選べるため、対象選択へそのまま進める。 */
    /* 最初の攻撃可能対象へカーソルを移し、対象選択状態へ進む。 */
    game->cursorX = game->units[target].x;
    game->cursorY = game->units[target].y;
    game->phase = PHASE_SELECT_TARGET;
    gameSetMessage(game, "こうげきできるてきをえらぶ B:もどる");
}

/* PHASE_SELECT_TARGET中の攻撃対象決定処理。 */
static void gameUpdateSelectTarget(Game *game, GameInput input)
{
    int target;
    Unit *attacker;
    Unit *defender;

    /* Bなら攻撃せず、カーソルを攻撃者へ戻して行動メニューへ戻る。 */
    if (input.cancel) {
        attacker = &game->units[game->selectedUnit];
        game->cursorX = attacker->x;
        game->cursorY = attacker->y;
        game->phase = PHASE_SELECT_ACTION;
        gameSetMessage(game, "こうげき または たいき");
        return;
    }
    if (!input.confirm) return;

    /* カーソル位置の番号と攻撃可否を再検証する。 */
    target = boardUnitAt(game, game->cursorX, game->cursorY);
    if (!boardCanAttack(game, game->selectedUnit, target)) {
        gameSetMessage(game, "こうげきできるてきをえらぶ");
        return;
    }

    attacker = &game->units[game->selectedUnit];
    defender = &game->units[target];
    /* -= は「左辺から右辺を引いて左辺へ代入」の省略形。 */
    defender->hp -= attacker->attack;
    /* HPが負数のまま表示されないよう0へ丸め、盤面から除外する。 */
    if (defender->hp <= 0) {
        defender->hp = 0;
        defender->alive = false;
    }
    gameFinishAction(game);
}

/* 新しい試合を開始できる完全な初期状態を作る。再戦時にも再利用する。 */
void gameInit(Game *game)
{
    int i;

    /*
     * memsetはgameが指すメモリ全体を0で埋める（5・7章）。
     * sizeof(*game)はポインタの大きさではなく、ポインタ先Gameの大きさ。
     */
    memset(game, 0, sizeof(*game));
    boardInit(game);
    /* i=0,1,2をA,B,Cとして両陣営へ配置する。キャストでintをUnitTypeへ変換。 */
    for (i = 0; i < TEAM_SIZE; i++) {
        /* P1は下段y=5、x=2,3,4。 */
        unitInit(&game->units[i], (UnitType)i, PLAYER_ONE, i + 2, BOARD_HEIGHT - 1);
        /* P2は配列後半3〜5、上段y=0。 */
        unitInit(&game->units[i + TEAM_SIZE], (UnitType)i, PLAYER_TWO, i + 2, 0);
    }

    game->currentPlayer = PLAYER_ONE;
    game->winner = PLAYER_NONE;
    game->phase = PHASE_SELECT_UNIT;
    game->selectedAction = ACTION_ATTACK;
    game->selectedUnit = -1;
    gameFocusFirstAvailableUnit(game);
    gameSetMessage(game, "プレイヤー1 キャラをえらぶ");
}

/* main.cから毎フレーム1回呼ばれる、ゲーム進行の公開入口。 */
void gameUpdate(Game *game, GameInput input)
{
    /* ゲーム終了中はSTART以外を無視し、押されたら同じGameを再初期化する。 */
    if (game->phase == PHASE_GAME_OVER) {
        if (input.restart) gameInit(game);
        return;
    }

    /*
     * 行動メニュー中の方向キーはメニュー専用にする。
     * それ以外の状態だけ盤面カーソルを動かすことで、黄色枠の誤移動を防ぐ。
     */
    if (game->phase != PHASE_SELECT_ACTION) {
        gameMoveCursor(game, input);
    }
    /* 現在のphaseだけに入力を渡す。これが状態機械の中心（10章の発展）。 */
    switch (game->phase) {
        case PHASE_SELECT_UNIT: gameUpdateSelectUnit(game, input); break;
        case PHASE_SELECT_MOVE: gameUpdateSelectMove(game, input); break;
        case PHASE_SELECT_ACTION: gameUpdateSelectAction(game, input); break;
        case PHASE_SELECT_TARGET: gameUpdateSelectTarget(game, input); break;
        case PHASE_GAME_OVER: break;
    }
}
