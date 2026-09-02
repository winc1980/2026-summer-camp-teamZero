/*
 * japanese_text.h — 日本語文字描画の公開窓口
 * --------------------------------------------------------------------------
 * UTF-8文字列を、8×8ドット文字として16bit背景へ描く関数を宣言している。
 * 呼び出し側はVRAMの先頭、表示座標、文字列、DSの16bit色を渡す。
 * 文章の配置はrender.c、文字データとUTF-8処理はjapanese_text.cが担当すｒ。
 *
 * 参考: 事前資料 4章（ポインタ）、5章（文字列）、8章（ビット演算）
 */
#ifndef JAPANESE_TEXT_H
#define JAPANESE_TEXT_H

#include <nds.h>

/*
 * pixels: 256px幅の描画先VRAM、x/y: 左上座標、text: UTF-8文字列、color: DS色。
 */
void japaneseTextDraw(u16 *pixels, int x, int y, const char *text, u16 color);

#endif /* JAPANESE_TEXT_H */
