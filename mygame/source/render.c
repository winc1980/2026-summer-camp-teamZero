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

/* Unitのalive/actedから、下画面の状態文字列を優先順に決める。 */
static const char *unitStatus(const Unit *unit)
{
    if (!unit->alive) return "たおれた";
    if (unit->acted) return "こうどうずみ";
    return "みこうどう";
}

/* 下画面の見える256×192ピクセルを黒で初期化。 */
static void clearUi(void)
{
    int i;
    for (i = 0; i < 256 * 192; i++) uiPixels[i] = makeColor(0, 0, 0);
}

/* Gameの状態を日本語の情報画面として下画面へ描く。 */
static void renderStatusScreen(const Game *game)
{
    int i;
    /* snprintfで1行を組み立てるための作業用C文字列（5章）。 */
    char line[96];
    u16 white = makeColor(31, 31, 31);
    u16 yellow = makeColor(31, 28, 2);
    /* 選べない項目を示す暗い灰色。新しい画像やパレットは使用しない。 */
    u16 disabled = makeColor(10, 10, 10);
    u16 blue = makeColor(8, 18, 31);
    u16 red = makeColor(31, 9, 9);

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

    /* 左上(x,y)と色を指定し、上から8〜12px間隔で各行を配置。 */
    japaneseTextDraw(uiPixels, 4, 4, "TACTICS MVP", yellow);
    /* %dへプレイヤー番号、%sへphaseNameの文字列を埋める（5章）。 */
    snprintf(line, sizeof(line), "プレイヤー%d  %s",
             (int)game->currentPlayer + 1, phaseName(game->phase));
    japaneseTextDraw(uiPixels, 4, 16, line, white);
    japaneseTextDraw(uiPixels, 4, 28, game->message, white);

    /* 行動選択中だけ、>と黄色で選択中項目を示す。 */
    if (game->phase == PHASE_SELECT_ACTION) {
        bool canAttack = game->selectedUnit >= 0 &&
            boardFindFirstAttackTarget(game, game->selectedUnit) >= 0;
        snprintf(line, sizeof(line), "%c こうげき",
                 canAttack && game->selectedAction == ACTION_ATTACK ? '>' : ' ');
        /* 攻撃対象がいない場合も文字は残し、暗い灰色で選択不可だと示す。 */
        japaneseTextDraw(uiPixels, 12, 40, line,
                         !canAttack ? disabled :
                         game->selectedAction == ACTION_ATTACK ? yellow : white);
        snprintf(line, sizeof(line), "%c たいき",
                 game->selectedAction == ACTION_WAIT ? '>' : ' ');
        japaneseTextDraw(uiPixels, 12, 48, line,
                         game->selectedAction == ACTION_WAIT ? yellow : white);
    }

    /* P1の3体はunits[0..2]、P2はunits[3..5]。 */
    japaneseTextDraw(uiPixels, 4, 64, "P1 (あお)", blue);
    for (i = 0; i < TEAM_SIZE; i++) {
        const Unit *unit = &game->units[i];
        /* %c=種類文字、%3d=最低3桁幅のHP、%s=日本語状態。 */
        snprintf(line, sizeof(line), " %c HP:%3d %s",
                 unitTypeLetter(unit->type), unit->hp, unitStatus(unit));
        japaneseTextDraw(uiPixels, 8, 72 + i * 8, line, white);
    }
    japaneseTextDraw(uiPixels, 4, 104, "P2 (あか)", red);
    for (i = TEAM_SIZE; i < UNIT_COUNT; i++) {
        const Unit *unit = &game->units[i];
        snprintf(line, sizeof(line), " %c HP:%3d %s",
                 unitTypeLetter(unit->type), unit->hp, unitStatus(unit));
        japaneseTextDraw(uiPixels, 8, 112 + (i - TEAM_SIZE) * 8, line, white);
    }
    japaneseTextDraw(uiPixels, 4, 152, "じゅうじ:カーソル A:けってい", white);
    japaneseTextDraw(uiPixels, 4, 164, "B:もどる START:もういちど", white);
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
