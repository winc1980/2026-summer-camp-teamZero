/*
 * input.c — DSの物理ボタンをゲーム用の入力へ変換する
 * --------------------------------------------------------------------------
 * libndsのkeysDown()が返すビット列を読み、GameInput構造体へ変換します。
 * game.cはKEY_AなどのDS固有定数を直接使わず、「決定」「取消」「上下移動」
 * という意味だけを受け取ります。
 *
 * 現在は、両プレイヤーが十字キー・A・B・STARTを共有し、ターンごとに
 * 本体を交代で操作する方式です。将来、左右でキーを分ける場合は、
 * 主にこのファイルでGameInputへの割り当てを変更します。
 *
 * 呼び出し順:
 * 1. main.cのscanKeys()がlibnds内部のキー状態を更新する
 * 2. inputReadShared()がkeysDown()を読み、GameInputを返す
 * 3. main.cがそのGameInputをgameUpdate()へ渡す
 *
 * 参考資料:
 * - 事前資料 6章: 複数の入力をGameInput構造体へまとめる
 * - 事前資料 8章: down & KEY_Aによるビット判定、8.5「DSのキー入力」
 * - 事前資料 12章: BlocksDS/libndsの開発環境
 * - BlocksDS公式 User input
 *   https://blocksds.skylyrac.net/tutorial/basic/input/
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
