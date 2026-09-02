/*
 * input.c
 * --------------------------------------------------------------------------
 * libndsが返すボタンのビット列を、ゲーム用GameInputへ翻訳する。
 * 現在は両プレイヤーが十字キー/A/B/STARTを共有して本体を交代で持つ。
 *
 * 事前資料との対応:
 * - 6章「構造体」   : 複数の入力をGameInputへまとめる
 * - 8章「ビット演算」: down & KEY_A で特定ビットだけを調べる
 * - 12章「開発環境」: BlocksDS/libndsでビルドする部分
 * - keysDown()等のlibnds APIは事前資料の本文外。BlocksDS公式資料の領域
 */

/* u32、keysDown、KEY_*を提供するNintendo DS用ヘッダ。 */
#include <nds.h>

#include "input.h"

/* 引数voidは「引数なし」。構造体GameInputをコピーして戻り値にできる。 */
GameInput inputReadShared(void)
{
    /*
     * keysDown()は、押し続けではなく、このフレームで新しく押されたキーを
     * 32ビット整数の各ビットとして返す。scanKeys()はmain.cで直前に呼ぶ。
     */
    u32 down = keysDown();
    /* {0}は全メンバを0=falseで初期化する構造体初期化構文（6章）。 */
    GameInput input = {0};

    /*
     * & はここではアドレス演算子ではなくビットAND（8章）。
     * 対応ビットが立っていれば結果が0以外になり、!= 0でboolへ変換する。
     */
    input.up = (down & KEY_UP) != 0;
    input.down = (down & KEY_DOWN) != 0;
    input.left = (down & KEY_LEFT) != 0;
    input.right = (down & KEY_RIGHT) != 0;
    input.confirm = (down & KEY_A) != 0;
    input.cancel = (down & KEY_B) != 0;
    input.restart = (down & KEY_START) != 0;
    /* ローカル変数inputの全メンバが呼び出し元へ値としてコピーされる。 */
    return input;
}
