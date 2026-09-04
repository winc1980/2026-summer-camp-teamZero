/*
 * input.h — ゲーム側が受け取る入力形式
 * --------------------------------------------------------------------------
 * DSのボタン名ではなく、ゲーム内での意味をboolでまとめたGameInputを
 * 定義している。game.cをハードウェアから切り離す境界になるヘッダです。
 * キー配置を変更してもGameInputの意味を保てば、ゲーム進行側は変更不要。
 *
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
