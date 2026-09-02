/*
 * input.h
 * DS固有のボタン名を、ゲーム側で使う意味（決定・取消など）へ変換する型。
 * game.cがKEY_A等を直接知らないため、将来の左右分割操作は主にinput.cだけで
 * 変更できる。構造体は6章、boolは2章、キーのビット判定は8章に対応する。
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

/* 1フレーム中に「新しく押された」操作をまとめる構造体。 */
typedef struct {
    bool up;      /* カーソルを上へ。 */
    bool down;    /* カーソルを下へ。 */
    bool left;    /* カーソルを左へ。 */
    bool right;   /* カーソルを右へ。 */
    bool confirm; /* Aボタン: 決定。 */
    bool cancel;  /* Bボタン: 1段階戻る。 */
    bool restart; /* START: 決着後の再試合。 */
} GameInput;

/* 現在の共通操作方式でDSキーを読み、GameInputを値として返す。 */
GameInput inputReadShared(void);

#endif /* INPUT_H */
