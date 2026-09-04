/*
 * main.c — アプリの開始地点と、1フレームごとの処理順
 * --------------------------------------------------------------------------
 * このファイルはゲーム全体の入口。起動時の初期化を行った後、
 * 「入力を読む → ゲーム状態を更新する → 画面を描画する」を繰り返す。
 * 移動・攻撃などのルールや具体的な描画処理は、担当ファイルへ任せてある。
 *
 * 最初に全体像をつかむときは、このファイルから読むのがおすすめ。
 * 呼び出し先は次のように分かれる。
 *  game.c   : ターン、選択状態、HP、勝敗を更新する
 *  input.c  : DSの物理ボタンをゲーム用の入力へ変換する
 *  render.c : Gameの内容を上下画面へ表示する
 *
 * 変更時の目安:
 *  1フレームの処理順を変える場合だけmain.cを変更する
 *  キャラクターのルール変更はboard.c / unit.cで行う
 *  画面や画像の変更はrender.cで行う
 *  キー配置の変更はinput.cで行う
 *
 * 参考:
 *  scanKeys()やVBlankはlibnds固有。BlocksDS公式User inputも参照
 *   https://blocksds.skylyrac.net/tutorial/basic/input/
 */

/* Nintendo DSの画面・キー・VBlank等を使うためのlibnds統合ヘッダ。 */
#include <nds.h>

#include "game.h"
#include "input.h"
#include "render.h"
#include "sound.h"

/*
 * OSから呼ばれる開始関数。
 * argc/argvは通常のCプログラムのコマンドライン引数だが、このゲームでは未使用。
 * char **argvは「文字列へのポインタを指すポインタ」。
 */
int main(int argc, char **argv) {
  /* 1試合分の状態をmain関数のローカル変数として確保する。 */
  Game game;
  /*1フレーム前の状態を確保*/
  Game preGame;

  /* (void)へキャストして、意図的な未使用引数だとコンパイラへ伝える。 */
  (void)argc;
  (void)argv;
  /* DSで例外が起きたときに診断画面を出し、無言停止を避ける */
  defaultExceptionHandler();
  /* VRAM、背景、スプライト、下画面を初期化 */
  renderInit();
  /*サウンド関連を設定*/
  soundInit();
  /* キャラ、盤面、ターン等を初期状態へ。&はgameのアドレス。 */
  gameInit(&game);
  /* 最初の比較で未初期化メモリを読まないよう、初期状態を保存する。 */
  preGame = game;
  /* 入力を待つ前に初期盤面を一度描画キューへ入れる。 */
  renderGame(&game);

  /* 1は常に真なので、DSアプリ終了まで繰り返す無限ループ。 */
  while (1) {
    /* 画面更新の境目まで待ち、描画のちらつきを抑える。 */
    swiWaitForVBlank();
    /* 前フレームに準備したスプライト情報を実画面へ反映。 */
    renderVBlank();
    /* DSのハードウェアキー状態を読み込み、keysDown()用に更新する。 */
    scanKeys();
    /* 物理キーを意味付き入力へ変換し、その1フレーム分だけゲームを進める。 */
    gameUpdate(&game, inputReadShared());
    /* 更新後の状態から次に表示する背景・駒・文字を準備。 */
    renderGame(&game);

    /*１フレーム前の試合と今の試合を渡し、その差からSEを出させる*/
    playAppropriateSE(&game, &preGame);
    preGame = game;
  }

  /* 無限ループから通常は到達しないが、mainの戻り値型intに合わせて書く。 */
  return 0;
}
