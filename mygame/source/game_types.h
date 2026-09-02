/*
 * game_types.h
 * --------------------------------------------------------------------------
 * ゲーム全体で共有する「定数」「列挙型(enum)」「構造体(struct)」を定義する。
 * ここには処理を書かず、ゲームの状態をどんなデータで表すかだけを集めている。
 *
 * 事前資料との対応:
 * - 2章「型と数値」       : int、bool、定数の考え方
 * - 5章「配列と文字列」   : terrain[][]、units[]、message[]
 * - 6章「構造体」         : Unit と Game に関連データをまとめる方法
 * - 10章「ゲームコードの型」: ゲーム状態を一つの構造体で管理する設計
 * - enum による状態遷移の設計は10章に近いが、この具体的な分け方は本作独自
 */

/*
 * インクルードガード。
 * 同じヘッダが複数回 #include されても、定義が重複してコンパイルエラーに
 * ならないよう、最初の1回だけ中身を有効にするCの定番パターン。
 */
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

/* bool、true、falseをC言語で使えるようにする標準ヘッダ。 */
#include <stdbool.h>

/*
 * #define はコンパイル前に名前を数値へ置き換える。
 * 盤面はDSの256×192pxを32px単位に分けるため、8列×6行になる。
 */
#define BOARD_WIDTH 8
#define BOARD_HEIGHT 6
#define TILE_SIZE 32
#define TEAM_SIZE 3
/* 1チーム3体 × 2人。括弧により式として安全に展開される。 */
#define UNIT_COUNT (TEAM_SIZE * 2)
#define INITIAL_HP 100

/*
 * enumは、単なる0・1・-1に意味のある名前を付ける型。
 * PLAYER_NONEは「勝者がまだいない」を表す番兵値（特別な値）。
 */
typedef enum {
    PLAYER_ONE = 0,
    PLAYER_TWO = 1,
    PLAYER_NONE = -1
} Player;

/* キャラクターA・B・C。値は0、1、2の順に自動採番される。 */
typedef enum {
    UNIT_A = 0,
    UNIT_B,
    UNIT_C
} UnitType;

/*
 * 地形の種類。MVPでは全マスPLAINだが、後から画像や通行ルールを
 * 追加してもGame構造体を作り直さなくて済むよう先に型だけ用意している。
 */
typedef enum {
    TERRAIN_PLAIN = 0,
    TERRAIN_MOUNTAIN,
    TERRAIN_RIVER,
    TERRAIN_BUILDING
} TerrainType;

/*
 * 現在どの操作段階かを表すステート（状態）。
 * PHASE_SELECT_UNIT → MOVE → ACTION → TARGET のようにgame.cで遷移する。
 * この「状態機械」の具体的な実装は事前資料外だが、10章のゲームループを
 * 複数画面・複数操作へ発展させたもの。
 */
typedef enum {
    PHASE_SELECT_UNIT = 0,
    PHASE_SELECT_MOVE,
    PHASE_SELECT_ACTION,
    PHASE_SELECT_TARGET,
    PHASE_GAME_OVER
} GamePhase;

/* 行動メニュー。ITEMは将来追加用で、現在の画面にはまだ表示しない。 */
typedef enum {
    ACTION_ATTACK = 0,
    ACTION_WAIT,
    ACTION_ITEM
} ActionType;

/*
 * 盤面上のキャラクター1体分のデータ。
 * typedef struct { ... } Unit; により、以後は「struct ...」ではなく
 * 短い型名 Unit で変数を宣言できる（6章）。
 */
typedef struct {
    UnitType type; /* A・B・Cのどれか。 */
    Player owner;  /* どちらのプレイヤーが所有するか。 */
    int x;         /* 盤面の列。左端が0、右へ行くほど増える。 */
    int y;         /* 盤面の行。上端が0、下へ行くほど増える。 */
    int hp;        /* 残り体力。0になるとaliveをfalseにする。 */
    int attack;    /* 1回の攻撃で相手HPから引く値。 */
    bool alive;    /* 生存中ならtrue、倒されたらfalse。 */
    bool acted;    /* 現在の自分ターンですでに行動したか。 */
} Unit;

/*
 * ゲーム1試合の状態をすべて持つ構造体。
 * Gameへのポインタを各関数へ渡すことで、グローバル変数を増やさず、
 * 全処理が同じ試合データを読み書きできる（4章・6章・10章）。
 */
typedef struct {
    /* [行][列] の2次元配列。terrain[y][x]の順でアクセスする（5章）。 */
    TerrainType terrain[BOARD_HEIGHT][BOARD_WIDTH];
    /* 両チーム合計6体を連続した配列で管理する。 */
    Unit units[UNIT_COUNT];
    Player currentPlayer;       /* 現在操作しているプレイヤー。 */
    Player winner;              /* 未決着はPLAYER_NONE、決着後は勝者。 */
    GamePhase phase;            /* 現在の操作段階。 */
    ActionType selectedAction;  /* 行動メニューで選択中の項目。 */
    int cursorX;                /* 黄色カーソルの列。 */
    int cursorY;                /* 黄色カーソルの行。 */
    int selectedUnit;           /* 選択中ユニットの配列番号。未選択は-1。 */
    int originX;                /* 仮移動をBで戻すために覚えておく元の列。 */
    int originY;                /* 仮移動をBで戻すために覚えておく元の行。 */
    /* C文字列は末尾の'\0'を含むchar配列（5章）。UTF-8日本語を保持する。 */
    char message[96];
} Game;

#endif /* GAME_TYPES_H */
