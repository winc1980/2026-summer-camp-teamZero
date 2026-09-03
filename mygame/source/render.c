/*
 * render.c — 上下画面の描画と、画像差し替えの中心
 * --------------------------------------------------------------------------
 * 上画面へ8×6盤面・ユニット・カーソル・移動/攻撃範囲を表示し、
 * 下画面へターン、案内文、HP、行動メニューを日本語で表示。
 * Gameはconstポインタで受け取り、描画中にルールやHPを変更しない。
 *
 * 現在の盤面とキャラクターは、画像素材が未確定でも遊べるように、
 * 16bitビットマップ背景と32×32スプライトをコード内で生成している。
 * （重要！）graphics/にある既存PNGはビルド対象として残しているが、このMVP描画には
 * まだ接続してない。画像担当が素材を組み込む際は、まず以下を確認。
 *
 * 画像変更の入口:
 *  terrainColor()/drawBoard() : 草・山・川・建造物など盤面の見た目
 *  makeUnitGraphics()          : P1/P2のA・B・Cの仮画像
 *  makeBorderGraphics()        : 黄色カーソル、水色移動範囲、赤色攻撃範囲
 *  renderStatusScreen()        : 下画面の文字・メニュー配置
 *  renderInit()                : 画面モード、VRAM、背景、OAMの初期化
 *
 * （重要！）黄色カーソルと範囲表示は、盤面やユニット画像へ直接描き込まず、
 * 中央が透明な別スプライトを上から重ねている。そのため、盤面・キャラ画像を
 * 差し替えても、32×32というマスサイズを保てば枠の仕組みを再利用できる。
 *
 * 参考資料:
 *  事前資料 2章: u16、色成分、キャスト
 *  事前資料 4章: VRAMを指すu16 *、読み取り専用のconst Game *
 *  事前資料 5章: ピクセル配列、snprintf、memcmp/memcpy
 *  事前資料 7章: VRAM、static配列、スプライト用メモリ
 *  事前資料 8章: ビット判定とDSの色
 *  事前資料 10章: 更新と描画を分けるゲームループ
 *  BlocksDS公式 Backgrounds
 *   https://blocksds.skylyrac.net/tutorial/basic/backgrounds/
 *  BlocksDS公式 Sprites
 *   https://blocksds.skylyrac.net/tutorial/basic/sprites/
 *  gritによる画像変換
 *   https://blocksds.skylyrac.net/grit/index.html
 */

/* libndsの画面・背景・スプライト・型を使うための統合ヘッダ。 */
#include <nds.h>
/* snprintfで表示行を安全に組み立てるためのC標準ヘッダ（5章）。 */
#include <stdio.h>
/* memcmp/memcpyで前回状態と比較・保存するための標準ヘッダ（5・7章）。 */
#include <string.h>

#include "board.h"
#include "japanese_text.h"
#include "render.h"
#include "unit.h"

#include "building_tile.h"
#include "mountain_tile.h"
#include "river_tile.h"


/*
 * OAMはDSのスプライト管理表。
 * スプライトごとに重ならないID範囲を予約し、毎フレーム同じ役割で再利用する。
 * enumに明示値を置くことで、ユニットやハイライト同士のID衝突を防ぐ。
 */
enum {
    OAM_CURSOR = 0,
    OAM_UNIT_BASE = 8,
    OAM_MOVE_BASE = 32,
    OAM_ATTACK_BASE = 40,
    OAM_HIGHLIGHT_MAX = 8
};

/*
 * staticグローバルはこの.cからだけ見える（3章）。
 * 背景IDはlibndsが返す管理番号、*PixelsはVRAM上の16bitピクセル配列を指す。
 */
static int boardBackground;
static int uiBackground;
static u16 *boardPixels;
static u16 *uiPixels;
/* [プレイヤー][A/B/C]ごとに32×32画像のVRAMアドレスを保持。 */
static u16 *unitGraphics[2][TEAM_SIZE];
static u16 *cursorGraphics;
static u16 *moveGraphics;
static u16 *attackGraphics;
/* 下画面を毎フレーム描き直さないための、前回表示したGameのコピー。 */
static Game lastConsoleGame;
static bool hasLastConsoleGame;
/* 盤面の黒緑点滅を防ぐため、前回描いた地形だけを別に保存する。 */
static TerrainType lastBoardTerrain[BOARD_HEIGHT][BOARD_WIDTH];
static bool hasLastBoardTerrain;

/* 0〜31のRGB成分を、DSの15bit色+不透明ビットを持つu16へ変換する。 */
static u16 makeColor(int r, int g, int b)
{
    /* ARGB16はlibndsのマクロ。先頭1は表示有効（不透明）を意味する。 */
    return ARGB16(1, r, g, b);
}

/* 32×32スプライト画像を透明色0で埋める。 */
static void clearSprite(u16 *graphics)
{
    int i;
    /* 1次元配列として全1024ピクセルを順番に初期化する（5章）。 */
    for (i = 0; i < TILE_SIZE * TILE_SIZE; i++) graphics[i] = 0;
}

/* 32×32画像の指定範囲を1色で塗る、簡易な長方形描画関数。 */
static void fillRect(u16 *graphics, int x, int y, int width, int height, u16 color)
{
    int row;
    int column;
    for (row = y; row < y + height; row++) {
        for (column = x; column < x + width; column++) {
            /* 範囲外へ書かないための境界チェック（5・11章）。 */
            if (column >= 0 && column < TILE_SIZE && row >= 0 && row < TILE_SIZE) {
                /* 2次元(row,column)を1次元配列番号row*幅+columnへ変換する。 */
                graphics[row * TILE_SIZE + column] = color;
            }
        }
    }
}

/*
 * A/B/Cを5×7ドットで表すビット列を返す。
 * unsigned charは8bitの符号なし整数（2・8章）。各ビットが1画素に対応する。
 */
static const unsigned char *glyphFor(UnitType type)
{
    /* static配列なので関数終了後も消えず、その先頭アドレスを安全に返せる（3・7章）。 */
    static const unsigned char glyphA[7] = {14, 17, 17, 31, 17, 17, 17};
    static const unsigned char glyphB[7] = {30, 17, 17, 30, 17, 17, 30};
    static const unsigned char glyphC[7] = {14, 17, 16, 16, 16, 17, 14};

    if (type == UNIT_A) return glyphA;
    if (type == UNIT_B) return glyphB;
    return glyphC;
}

/* 5×7の1ドットを3×3ピクセルへ拡大し、駒の中央に描く。 */
static void drawLetter(u16 *graphics, UnitType type, u16 color)
{
    const unsigned char *glyph = glyphFor(type);
    int row;
    int column;
    int scale = 3;
    int offsetX = 8;
    int offsetY = 5;

    for (row = 0; row < 7; row++) {
        for (column = 0; column < 5; column++) {
            /*
             * 1 << (4-column)で調べたい位置だけ1のマスクを作り、
             * &でそのドットが立っているか判定する（8章）。
             */
            if (glyph[row] & (1 << (4 - column))) {
                fillRect(graphics, offsetX + column * scale, offsetY + row * scale,
                         scale, scale, color);
            }
        }
    }
}

/* プレイヤー色の本体・白枠・黄色文字を組み合わせて駒画像を作る。 */
static void makeUnitGraphics(u16 *graphics, Player owner, UnitType type)
{
    /* P1は青、P2は赤。三項演算子で色を選ぶ。 */
    u16 body = owner == PLAYER_ONE ? makeColor(5, 13, 30) : makeColor(30, 7, 7);
    clearSprite(graphics);
    fillRect(graphics, 3, 3, 26, 26, makeColor(31, 31, 31));
    fillRect(graphics, 5, 5, 22, 22, body);
    drawLetter(graphics, type, makeColor(31, 28, 2));
}

/* カーソルや移動/攻撃範囲用の枠画像を作る。checker=trueなら市松模様も描く。 */
static void makeBorderGraphics(u16 *graphics, u16 color, bool checker)
{
    int x;
    int y;
    clearSprite(graphics);
    if (checker) {
        for (y = 2; y < 30; y++) {
            for (x = 2; x < 30; x++) {
                /* /4で4px単位のマスにし、%2の偶奇で市松模様を作る。 */
                if (((x / 4) + (y / 4)) % 2 == 0) {
                    graphics[y * TILE_SIZE + x] = color;
                }
            }
        }
    }
    /* 上・下・左・右の順に太さ2pxの枠を描く。 */
    fillRect(graphics, 0, 0, TILE_SIZE, 2, color);
    fillRect(graphics, 0, TILE_SIZE - 2, TILE_SIZE, 2, color);
    fillRect(graphics, 0, 0, 2, TILE_SIZE, color);
    fillRect(graphics, TILE_SIZE - 2, 0, 2, TILE_SIZE, color);
}

/* 地形enumを仮表示色へ変換する。最終画像へ差し替えるまでの表示。 */
static u16 terrainColor(TerrainType terrain)
{
    switch (terrain) {
        case TERRAIN_MOUNTAIN: return makeColor(17, 12, 7);
        case TERRAIN_RIVER: return makeColor(4, 16, 28);
        case TERRAIN_BUILDING: return makeColor(19, 19, 19);
        case TERRAIN_PLAIN:
        default: return makeColor(7, 22, 8);
    }
}

/* 16bitビットマップ背景へ、黒い罫線付きの8×6盤面を直接描く。 */
static void drawBoard(const Game *game)
{
    int x;
    int y;
    int pixelX;
    int pixelY;

    /* 256×256背景を一度黒くする。実際に見える高さは上側192px。 */
    for (pixelY = 0; pixelY < 256; pixelY++) {
        for (pixelX = 0; pixelX < 256; pixelX++) {
            boardPixels[pixelY * 256 + pixelX] = makeColor(0, 0, 0);
        }
    }

    /* 各地形マスの内側30×30pxを塗り、外周1pxの黒を罫線として残す。 */
    for (y = 0; y < BOARD_HEIGHT; y++) {
        for (x = 0; x < BOARD_WIDTH; x++) {
            u16 color = terrainColor(game->terrain[y][x]);
            /* マス座標を画面ピクセル座標へ変換。 */
            int top = y * TILE_SIZE;
            int left = x * TILE_SIZE;
            for (pixelY = top + 1; pixelY < top + TILE_SIZE - 1; pixelY++) {
                for (pixelX = left + 1; pixelX < left + TILE_SIZE - 1; pixelX++) {
                    boardPixels[pixelY * 256 + pixelX] = color;
                }
            }
        }
    }
}

/*
 * libndsのoamSetへ、このゲームで共通の32×32ビットマップ設定を渡す。
 * id=OAM番号、priorityは小さいほど手前、alphaは0〜15の透明度。
 * 引数の多いoamSetを直接各所へ描かず、間違いを減らすラッパー関数。
 */
static void setBitmapSprite(int id, int x, int y, int priority, int alpha, u16 *graphics)
{
    oamSet(&oamMain, id, x, y, priority, alpha, SpriteSize_32x32,
           SpriteColorFormat_Bmp, graphics, -1, false, false, false, false, false);
}

/* 現在phaseに応じ、移動可能マスまたは攻撃可能な敵へ半透明枠を置く。 */
static void renderHighlights(const Game *game)
{
    int x;
    int y;
    int count = 0;

    /* ユニット未選択なら判定に使える番号がないので何も描かない。 */
    if (game->selectedUnit < 0) return;
    if (game->phase == PHASE_SELECT_MOVE) {
        /* 全48マスをboardCanMoveToへ渡し、trueのマスだけ水色表示。 */
        for (y = 0; y < BOARD_HEIGHT && count < OAM_HIGHLIGHT_MAX; y++) {
            for (x = 0; x < BOARD_WIDTH && count < OAM_HIGHLIGHT_MAX; x++) {
                if (boardCanMoveTo(game, game->selectedUnit, x, y)) {
                    /* マス座標×32でスプライトの左上ピクセル座標になる。 */
                    setBitmapSprite(OAM_MOVE_BASE + count, x * TILE_SIZE, y * TILE_SIZE,
                                    2, 6, moveGraphics);
                    count++;
                }
            }
        }
    } else if (game->phase == PHASE_SELECT_TARGET) {
        int i;
        /* 生存6体から攻撃可能な敵だけを赤く表示する。 */
        for (i = 0; i < UNIT_COUNT && count < OAM_HIGHLIGHT_MAX; i++) {
            if (boardCanAttack(game, game->selectedUnit, i)) {
                setBitmapSprite(OAM_ATTACK_BASE + count,
                                game->units[i].x * TILE_SIZE,
                                game->units[i].y * TILE_SIZE,
                                2, 8, attackGraphics);
                count++;
            }
        }
    }
}

/* 生存中の全ユニットを、所有者色と種類に対応する画像で表示する。 */
static void renderUnits(const Game *game)
{
    int i;
    for (i = 0; i < UNIT_COUNT; i++) {
        const Unit *unit = &game->units[i];
        /* continueは現在のループだけ飛ばし、次のiへ進む。死亡者は描かない。 */
        if (!unit->alive) continue;
        /* actedならalpha=9に下げ、行動済みを少し薄く見せる。 */
        setBitmapSprite(OAM_UNIT_BASE + i, unit->x * TILE_SIZE, unit->y * TILE_SIZE,
                        1, unit->acted ? 9 : 15,
                        unitGraphics[unit->owner][unit->type]);
    }
}

/*
 * 地形配列が前回と違うときだけ盤面背景を描き直す。
 * 最初に作ったとき、毎フレーム「黒く消す→緑を塗る」を行い、LCDが途中状態を読んで
 * 点滅してしまったので、memcmpで同一ならreturnし、静止中のVRAM書込みをなくしてる。
 */
static void drawBoardIfChanged(const Game *game)
{
    /* memcmpは指定バイト数が全て同じなら0を返す（5・7章）。 */
    if (hasLastBoardTerrain &&
        memcmp(lastBoardTerrain, game->terrain, sizeof(lastBoardTerrain)) == 0) {
        return;
    }
    drawBoard(game);
    /* memcpyで今回の地形配列を比較用配列へ丸ごとコピー。 */
    memcpy(lastBoardTerrain, game->terrain, sizeof(lastBoardTerrain));
    hasLastBoardTerrain = true;
}

/* 画面に表示する日本語のphase名を返す。 */
static const char *phaseName(GamePhase phase)
{
    switch (phase) {
        case PHASE_SELECT_UNIT: return "キャラせんたく";
        case PHASE_SELECT_MOVE: return "いどうさきせんたく";
        case PHASE_SELECT_ACTION: return "こうどうせんたく";
        case PHASE_SELECT_TARGET: return "てきせんたく";
        case PHASE_GAME_OVER: return "ゲームしゅうりょう";
        default: return "?";
    }
}

/* 下画面の見える256×192ピクセルを黒で初期化。 */
static void clearUi(void)
{
    int i;
    for (i = 0; i < 256 * 192; i++) uiPixels[i] = makeColor(0, 0, 0);
}

/* 下画面の指定範囲を塗る。画面外は切り捨て、VRAMの範囲外へ書かない。 */
static void fillUiRect(int x, int y, int width, int height, u16 color)
{
    int row;
    int column;

    for (row = y; row < y + height; row++) {
        for (column = x; column < x + width; column++) {
            if (column >= 0 && column < 256 && row >= 0 && row < 192) {
                uiPixels[row * 256 + column] = color;
            }
        }
    }
}

/* 塗りつぶし長方形の外周へ1pxの枠を描く。 */
static void drawUiFrame(int x, int y, int width, int height, u16 color)
{
    fillUiRect(x, y, width, 1, color);
    fillUiRect(x, y + height - 1, width, 1, color);
    fillUiRect(x, y, 1, height, color);
    fillUiRect(x + width - 1, y, 1, height, color);
}

/* 攻撃ボタンへ、右上向きの剣を単純な図形で描く。 */
static void drawAttackIcon(int x, int y, u16 color)
{
    int i;

    for (i = 0; i < 13; i++) {
        fillUiRect(x + 4 + i, y + 16 - i, 2, 2, color);
    }
    fillUiRect(x + 2, y + 17, 9, 2, color);
    fillUiRect(x + 4, y + 19, 2, 5, color);
    fillUiRect(x + 16, y + 2, 4, 2, color);
    fillUiRect(x + 18, y + 2, 2, 4, color);
}

/* 待機ボタンへ砂時計を単純な図形で描く。 */
static void drawWaitIcon(int x, int y, u16 color)
{
    int i;

    fillUiRect(x + 3, y + 3, 16, 2, color);
    fillUiRect(x + 3, y + 20, 16, 2, color);
    for (i = 0; i < 7; i++) {
        fillUiRect(x + 5 + i, y + 5 + i, 2, 2, color);
        fillUiRect(x + 15 - i, y + 5 + i, 2, 2, color);
        fillUiRect(x + 11 - i, y + 12 + i, 2, 2, color);
        fillUiRect(x + 9 + i, y + 12 + i, 2, 2, color);
    }
}

typedef enum {
    UI_BUTTON_DISABLED = 0,
    UI_BUTTON_IDLE,
    UI_BUTTON_SELECTED,
    UI_BUTTON_CONFIRMED
} UiButtonState;

/* 枠、色、アイコン、押し込み位置をまとめて1つの行動ボタンを描く。 */
static void drawActionButton(int x, int y, int width, const char *label,
                             bool attackButton, UiButtonState state)
{
    int offset = state == UI_BUTTON_SELECTED ? 1 :
                 state == UI_BUTTON_CONFIRMED ? 2 : 0;
    u16 shadow = makeColor(2, 2, 3);
    u16 border = state == UI_BUTTON_SELECTED ? makeColor(31, 28, 2) :
                 state == UI_BUTTON_CONFIRMED ? makeColor(31, 31, 31) :
                 state == UI_BUTTON_DISABLED ? makeColor(10, 10, 10) :
                 makeColor(22, 22, 24);
    u16 fill = state == UI_BUTTON_DISABLED ? makeColor(6, 6, 7) :
               attackButton ?
                   (state == UI_BUTTON_SELECTED ? makeColor(25, 6, 4) : makeColor(14, 4, 4)) :
                   (state == UI_BUTTON_SELECTED ? makeColor(4, 12, 26) : makeColor(3, 7, 15));
    u16 foreground = state == UI_BUTTON_DISABLED ? makeColor(10, 10, 10) :
                     makeColor(31, 31, 31);

    fillUiRect(x + 2, y + 3, width, 32, shadow);
    fillUiRect(x, y + offset, width, 32, fill);
    drawUiFrame(x, y + offset, width, 32, border);
    if (state == UI_BUTTON_SELECTED) {
        drawUiFrame(x + 1, y + offset + 1, width - 2, 30, border);
    }
    if (attackButton) drawAttackIcon(x + 7, y + offset + 3, foreground);
    else drawWaitIcon(x + 7, y + offset + 3, foreground);
    japaneseTextDraw(uiPixels, x + 35, y + offset + 12, label, foreground);
}

/* 選択確定済みのキャラ、またはカーソルを合わせている自軍キャラを返す。 */
static int statusUnitIndex(const Game *game)
{
    int index;

    if (game->selectedUnit >= 0 && game->selectedUnit < UNIT_COUNT &&
        game->units[game->selectedUnit].alive) {
        return game->selectedUnit;
    }
    index = boardUnitAt(game, game->cursorX, game->cursorY);
    if (index >= 0 && game->units[index].owner == game->currentPlayer) return index;
    return -1;
}

/* 選択中キャラを中央に置き、実際の盤面ルールから5×5の攻撃範囲図を描く。 */
static void drawAttackRangeMap(const Game *game, int unitIndex, int left, int top)
{
    const Unit *unit = &game->units[unitIndex];
    int mapY;
    int mapX;
    u16 grid = makeColor(10, 12, 16);
    u16 empty = makeColor(3, 4, 7);
    u16 outside = makeColor(1, 1, 2);
    u16 range = makeColor(24, 5, 4);
    u16 unitColor = unit->owner == PLAYER_ONE ? makeColor(7, 17, 31) :
                                                makeColor(31, 7, 7);

    for (mapY = -2; mapY <= 2; mapY++) {
        for (mapX = -2; mapX <= 2; mapX++) {
            int boardX = unit->x + mapX;
            int boardY = unit->y + mapY;
            int cellX = left + (mapX + 2) * 10;
            int cellY = top + (mapY + 2) * 10;
            u16 fill = boardIsInside(boardX, boardY) ? empty : outside;

            if (boardCanAttackFrom(game, unitIndex, unit->x, unit->y, boardX, boardY)) {
                fill = range;
            }
            if (mapX == 0 && mapY == 0) fill = unitColor;
            fillUiRect(cellX, cellY, 10, 10, grid);
            fillUiRect(cellX + 1, cellY + 1, 8, 8, fill);
        }
    }
}

/* 1チーム3体のHPを、常時確認できる小さな1行へまとめる。 */
static void drawTeamSummary(const Game *game, Player owner, int x, int y, u16 color)
{
    int base = owner == PLAYER_ONE ? 0 : TEAM_SIZE;
    char line[40];

    snprintf(line, sizeof(line), "P%d A%3d B%3d C%3d", (int)owner + 1,
             game->units[base].hp, game->units[base + 1].hp, game->units[base + 2].hp);
    japaneseTextDraw(uiPixels, x, y, line, color);
}

/* Gameの状態を日本語の情報画面として下画面へ描く。 */
static void renderStatusScreen(const Game *game)
{
    int unitIndex;
    char line[96];
    u16 white = makeColor(31, 31, 31);
    u16 yellow = makeColor(31, 28, 2);
    u16 blue = makeColor(8, 18, 31);
    u16 red = makeColor(31, 9, 9);
    u16 panel = makeColor(3, 5, 9);
    bool actionPhase = game->phase == PHASE_SELECT_ACTION;
    bool targetPhase = game->phase == PHASE_SELECT_TARGET;
    bool canAttack = game->selectedUnit >= 0 &&
        boardFindFirstAttackTarget(game, game->selectedUnit) >= 0;
    UiButtonState attackState = UI_BUTTON_DISABLED;
    UiButtonState waitState = UI_BUTTON_DISABLED;

    /*
     * （重要！）Game全体が前回と同じなら描画しない。毎フレームclearUiすると文字が
     * ちらつくため、状態が変わったフレームだけ更新する。
     */
    if (hasLastConsoleGame && memcmp(&lastConsoleGame, game, sizeof(*game)) == 0) {
        return;
    }
    memcpy(&lastConsoleGame, game, sizeof(*game));
    hasLastConsoleGame = true;
    clearUi();

    /* 上端は現在の手番、操作段階、ゲームからの案内文。 */
    snprintf(line, sizeof(line), "P%d  %s", (int)game->currentPlayer + 1,
             phaseName(game->phase));
    japaneseTextDraw(uiPixels, 4, 3, line, yellow);
    japaneseTextDraw(uiPixels, 4, 14, game->message, white);

    /* 行動選択中だけ操作可能にし、選択中のボタンを明るく押し込んで表示。 */
    if (actionPhase) {
        if (canAttack) {
            attackState = game->selectedAction == ACTION_ATTACK ?
                UI_BUTTON_SELECTED : UI_BUTTON_IDLE;
        }
        waitState = game->selectedAction == ACTION_WAIT ? UI_BUTTON_SELECTED : UI_BUTTON_IDLE;
    } else if (targetPhase) {
        attackState = UI_BUTTON_CONFIRMED;
    }
    drawActionButton(6, 27, 119, "こうげき", true, attackState);
    drawActionButton(131, 27, 119, "たいき", false, waitState);

    /* 左に攻撃範囲図、中央に選択キャラ情報、右に全員分の小さなHP。 */
    unitIndex = statusUnitIndex(game);
    fillUiRect(5, 67, 246, 69, panel);
    drawUiFrame(5, 67, 246, 69, makeColor(12, 14, 19));
    if (unitIndex >= 0) {
        const Unit *unit = &game->units[unitIndex];
        drawAttackRangeMap(game, unitIndex, 10, 76);
        snprintf(line, sizeof(line), "P%d %c", (int)unit->owner + 1,
                 unitTypeLetter(unit->type));
        japaneseTextDraw(uiPixels, 66, 76, line,
                         unit->owner == PLAYER_ONE ? blue : red);
        snprintf(line, sizeof(line), "ATK %d", unit->attack);
        japaneseTextDraw(uiPixels, 66, 90, line, white);
        japaneseTextDraw(uiPixels, 66, 104, unitSkillName(unit->type), white);
        japaneseTextDraw(uiPixels, 66, 118, "RANGE", red);
    } else {
        japaneseTextDraw(uiPixels, 12, 90, "キャラをえらぶ", white);
    }
    drawTeamSummary(game, PLAYER_ONE, 158, 80, blue);
    drawTeamSummary(game, PLAYER_TWO, 158, 96, red);

    /* 次のHPゲージIssueで拡張する領域。現時点では数値だけを引き継ぐ。 */
    fillUiRect(5, 141, 246, 30, panel);
    drawUiFrame(5, 141, 246, 30, makeColor(12, 14, 19));
    if (unitIndex >= 0) {
        snprintf(line, sizeof(line), "HP %d", game->units[unitIndex].hp);
        japaneseTextDraw(uiPixels, 12, 152, line, white);
    } else {
        japaneseTextDraw(uiPixels, 12, 152, "HP ---", makeColor(10, 10, 10));
    }

    japaneseTextDraw(uiPixels, 5, 180, "A:けってい B:もどる", white);
}

/* DSの映像ハードウェアと、実行中に生成する仮画像を起動時に準備。 */
void renderInit(void)
{
    int owner;
    int type;

    /* メインエンジン（盤面）を上画面、サブエンジン（UI）を下画面へ割り当ててる。 */
    lcdMainOnTop();
    /* MODE_5_2Dは16bitビットマップ背景を使える2D画面モード（資料外）。 */
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    /*
     * DS内のVRAMバンクA〜Dの用途を決める。
     * A=上背景、B=上スプライト、C=下背景、D=未使用LCD領域。
     */
    vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000,
                        VRAM_B_MAIN_SPRITE,
                        VRAM_C_SUB_BG,
                        VRAM_D_LCD);

    /* 上画面に256×256の16bit背景を作り、VRAM先頭ポインタを取得する。 */
    boardBackground = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgSetPriority(boardBackground, 3);
    boardPixels = bgGetGfxPtr(boardBackground);

    /* 下画面も同形式にし、日本語フォントをピクセル単位で描けるようにする。 */
    uiBackground = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgSetPriority(uiBackground, 3);
    uiPixels = bgGetGfxPtr(uiBackground);

    /* メイン画面OAMを16bitビットマップスプライト用に初期化する。 */
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    oamEnable(&oamMain);
    /* 2陣営×3種類=6枚の駒画像をVRAM Bから確保し、その場で生成する。 */
    for (owner = 0; owner < 2; owner++) {
        for (type = 0; type < TEAM_SIZE; type++) {
            unitGraphics[owner][type] = oamAllocateGfx(&oamMain, SpriteSize_32x32,
                                                       SpriteColorFormat_Bmp);
            /* owner/typeはintループ変数なのでenum型へ明示キャストする（2章）。 */
            makeUnitGraphics(unitGraphics[owner][type], (Player)owner, (UnitType)type);
        }
    }
    /* カーソル、移動範囲、攻撃範囲の3枚も同じ32×32で確保する。 */
    cursorGraphics = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
    moveGraphics = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
    attackGraphics = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
    makeBorderGraphics(cursorGraphics, makeColor(31, 31, 0), false);
    makeBorderGraphics(moveGraphics, makeColor(0, 25, 31), true);
    makeBorderGraphics(attackGraphics, makeColor(31, 5, 2), true);
}

/* 1フレームのGameから、次の画面内容をOAM/VRAMへ準備。 */
void renderGame(const Game *game)
{
    /* 静的な背景は変更時のみ更新する。 */
    drawBoardIfChanged(game);
    /* 前フレームのスプライト登録を一旦消し、現状態から登録し直す。 */
    oamClear(&oamMain, 0, 128);
    /* priorityはカーソル0、ユニット1、ハイライト2、背景3の手前順。 */
    renderHighlights(game);
    renderUnits(game);
    setBitmapSprite(OAM_CURSOR, game->cursorX * TILE_SIZE, game->cursorY * TILE_SIZE,
                    0, 15, cursorGraphics);
    renderStatusScreen(game);
}

/* VBlank中に、メモリ上で組み立てたOAMをDSハードウェアへ転送する。 */
void renderVBlank(void)
{
    oamUpdate(&oamMain);
}
