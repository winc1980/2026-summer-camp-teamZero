/*
 * japanese_text.h
 * UTF-8の日本語文字列を8×8ドット文字として16bit背景へ描く公開関数。
 * DS標準コンソールは日本語非対応なので作った、本作独自の小さな描画層。
 * ポインタは4章、文字列は5章、ビット処理は8章。UTF-8解析は資料外。
 */
#ifndef JAPANESE_TEXT_H
#define JAPANESE_TEXT_H

#include <nds.h>

/*
 * pixels: 256px幅の描画先VRAM、x/y: 左上座標、text: UTF-8文字列、color: DS色。
 */
void japaneseTextDraw(u16 *pixels, int x, int y, const char *text, u16 color);

#endif /* JAPANESE_TEXT_H */
