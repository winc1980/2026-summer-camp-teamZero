/*
 * main.c
 * --------------------------------------------------------------------------
 * DSアプリの開始地点（エントリーポイント）とゲームループだけを担当する。
 * 個別の移動・攻撃ルールは書かず、入力→更新→描画の順番を繰り返す。
 *
 * 事前資料との対応:
 * - 1章「Cの世界観」       : main関数、#include、コンパイル単位
 * - 3章「関数まわり」     : 関数呼び出し
 * - 4章「ポインタ」       : &gameで同じGameを各処理へ渡す
 * - 10章「ゲームコードの型」: while(1)のゲームループ
 * - 12章「開発環境」      : BlocksDSで.ndsを作る入口
 * - VBlank・scanKeys等のlibnds APIは事前資料外で、BlocksDS固有
 */

/* Nintendo DSの画面・キー・VBlank等を使うためのlibnds統合ヘッダ。 */
#include <nds.h>

#include "game.h"
#include "input.h"
#include "render.h"

/*
 * OSから呼ばれる開始関数。
 * argc/argvは通常のCプログラムのコマンドライン引数だが、このゲームでは未使用。
 * char **argvは「文字列へのポインタを指すポインタ」（4・5章）。
 */
int main(int argc, char **argv)
{
    /* 1試合分の状態をmain関数のローカル変数として確保する（6・7章）。 */
    Game game;

    /* (void)へキャストして、意図的な未使用引数だとコンパイラへ伝える。 */
    (void)argc;
    (void)argv;
    /* DSで例外が起きたときに診断画面を出し、無言停止を避ける。 */
    defaultExceptionHandler();
    /* VRAM、背景、スプライト、下画面を初期化する。 */
    renderInit();
    /* キャラ、盤面、ターン等を初期状態へする。&はgameのアドレス（4章）。 */
    gameInit(&game);
    /* 入力を待つ前に初期盤面を一度描画キューへ入れる。 */
    renderGame(&game);

    /* 1は常に真なので、DSアプリ終了まで繰り返す無限ループ（10章）。 */
    while (1) {
        /* 画面更新の境目まで待ち、描画のちらつきを抑える。 */
        swiWaitForVBlank();
        /* 前フレームに準備したOAM（スプライト情報）を実画面へ反映する。 */
        renderVBlank();
        /* DSのハードウェアキー状態を読み込み、keysDown()用に更新する。 */
        scanKeys();
        /* 物理キーを意味付き入力へ変換し、その1フレーム分だけゲームを進める。 */
        gameUpdate(&game, inputReadShared());
        /* 更新後の状態から次に表示する背景・駒・文字を準備する。 */
        renderGame(&game);
    }

    /* 無限ループから通常は到達しないが、mainの戻り値型intに合わせて書く。 */
    return 0;
}
