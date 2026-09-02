# チーム0のゲーム（仮）

合宿用に作った、Nintendo DS向けの2人対戦ターン制ゲームです。

上画面は `8 × 6` の盤面、下画面はターン・HP・操作案内です。現在は仮の色と文字で描画しているため、画像素材が未決定でもゲーム部分を試せます。

## ビルド

VS CodeでDev Containerを開き、ターミナルで次を実行します。

```bash
cd /work/mygame
make
```

成功すると `mygame.nds` ができます。`make: Nothing to be done for 'all'.` は、前回から変更がなくビルド済みという意味です。最初から作り直す場合は次を使います。

```bash
make clean
make
```

## 操作

- 十字キー：カーソル移動、攻撃/待機の切り替え
- A：決定
- B：1段階戻る。移動後ならキャラも元の位置に戻る
- START：ゲーム終了後に最初からやり直す

1台を交代で持つ共通操作方式です。各プレイヤーは自分のターンに、生きていて未行動の3体を1回ずつ動かします。

melonDSでA以外が未設定なら、`Config → Input and Hotkeys` の `DS keypad` で、Up/Down/Left/Rightをキーボードの矢印、AをX、BをZ、StartをEnterなどに割り当ててください。

下画面の案内は日本語表示です。8×8ドットの美咲フォントから、ゲームで使う文字だけを収録しています。

詳しいルールと変更箇所は [docs/team-cheatsheet.md](docs/team-cheatsheet.md) を参照してください。

## PC上のルールテスト

DSの描画を除くルールは、macOS側でも確認できます。

```bash
cd /work/mygame
cc -std=c11 -Wall -Wextra -Werror -Isource \
  test/test_rules.c source/game.c source/board.c source/unit.c \
  -o /tmp/mygame-rule-tests
/tmp/mygame-rule-tests
```

`All game rule tests passed.` と出れば成功です。
