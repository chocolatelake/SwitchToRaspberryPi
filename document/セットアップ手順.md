# Raspberry Pi Pico Switchコントローラー化 完全セットアップ手順書

本ドキュメントは、PC（C#のGUIアプリ）からRaspberry Pi Picoへシリアル通信でコマンドを送り、PicoをNintendo Switchのコントローラー（POKKEN CONTROLLER互換）として動作させるための環境構築・配線・トラブルシューティング手順をまとめたものです。
**※将来別のPCに移行した際や、AIアシスタントに文脈を共有する際は、このドキュメントを提示することでスムーズに開発を引き継げます。**

## 1. システムの全体構成と物理配線（超重要）
本システムは、PCから直接PicoへUSB通信するのではなく、**「USBシリアル変換器」を経由してPicoのピン（UART）へ通信**を行います。

**【正しい接続図】**
`[PC (C# GUIアプリ)]` ＝(USBケーブル)＝ `[USBシリアル変換器 (COMx)]` ＝(ジャンパーワイヤ3本)＝ `[Raspberry Pi Pico]` ＝(USBケーブル)＝ `[Switch本体 または ドック]`

**【ジャンパーワイヤの配線ルール（クロス接続）】**
シリアル通信は「話す口(TX)」と「聞く耳(RX)」を交差させて繋ぎます。**また、電気の基準を合わせるためGNDの接続は必須です。**
- USB変換器の **TX** ピン ➔ Picoの **GP1 (Pin 2 / UART0 RX)** へ
- USB変換器の **RX** ピン ➔ Picoの **GP0 (Pin 1 / UART0 TX)** へ
- USB変換器の **GND** ピン ➔ Picoの **GND** ピン (Pin 3 や Pin 38など) へ

## 2. Nintendo Switch側の事前設定
Picoを繋ぐ前に、Switch本体の設定を変更しないとコントローラーとして認識されません。
1. Switchホーム画面から **「設定」 ＞ 「コントローラーとセンサー」** を開く。
2. **「Proコントローラーの有線通信」** を **「ON」** にする。
3. 接続後、動かない場合は **「コントローラー」 ＞ 「持ち方/順番を変える」** 画面を開き、C#アプリからLRボタンなどを送信して認識させる。
4. スリープ運用で認識がバグった場合は、Switch本体を一度 **「再起動」** すると直ることが多い。

## 3. Arduino IDE の準備・ボード設定
公式のMbed版ではなく、より高機能な有志版（Earle F. Philhower版）を使用します。

1. Arduino公式サイトからArduino IDEをダウンロードしてインストールします。
2. **ボードマネージャのURLを追加**
   - 上部メニューから **「ファイル」 > 「基本設定」** を開きます。
   - **「追加のボードマネージャのURL」** に以下を入力し「OK」を押します。
     `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
3. **ボードデータのインストール**
   - **「ツール」 > 「ボード」 > 「ボードマネージャ」** を開きます。
   - 検索窓に `pico` と入力し、**「Raspberry Pi Pico/RP2040 by Earle F. Philhower, III」** をインストールします。
4. **ボードとUSB Stackの設定（超重要）**
   - **「ツール」 > 「ボード」 > 「Raspberry Pi Pico/RP2040」** の中から、**「Raspberry Pi Pico」** を選択します。
   - **「ツール」 > 「USB Stack」** を **「Adafruit TinyUSB」** に設定します。
   *※注意: このボードデータにはSwitch化に最適な「Adafruit TinyUSB」ライブラリが最初から内蔵されています。個別にインストールすると重複エラーの原因になります。*

## 4. プログラム（ソースコード）の準備と通信仕様
`switch_ctrl.ino` の主要な仕様と注意点です。

**【1. 通信方式の制限】**
PCとUSBシリアル変換器（TX/RXピン）を介して通信するため、待ち受けは**必ず `Serial1`** を使用します。（過去のAIが提案しがちな `Serial` への変更は**絶対NG**です。PCからの声が届かなくなります）
- `Serial1.setRX(1); Serial1.setTX(0); Serial1.begin(9600);`
- `if (Serial1.available() > 0) { char cmd = Serial1.read(); ... }`

**【2. C#アプリとの通信プロトコル（1文字暗号）】**
通信ラグを防ぐため、1文字の送信で同時押しや斜め移動を実現しています。
- `5` = L+R 同時押し
- `t` = X+Y 同時押し, `y` = X+A, `u` = Y+B, `i` = A+B
- `6` = 左上, `7` = 右上, `8` = 左下, `9` = 右下 (左スティック)
- `z`=A, `x`=B, `s`=X, `a`=Y, `q`=L, `w`=R, `I`=上, `J`=左, `K`=下, `L`=右

**【3. USBデバイス設定】**
使用するオブジェクト名は `TinyUSBDevice` です。
```cpp
TinyUSBDevice.setID(0x0F0D, 0x0092);
TinyUSBDevice.setProductDescriptor("POKKEN CONTROLLER");
TinyUSBDevice.setManufacturerDescriptor("HORI CO.,LTD.");
```

## 5. マイコンへの書き込み・よくあるエラー
1. **BOOTSELモードでPCに接続**
   - Pico基板上の **「BOOTSEL」という白いボタンを押したまま**、PCにUSBケーブルを接続します。（「RPI-RP2」として認識されたら離す）
2. **書き込み**
   - Arduino IDEで **「→（書き込み）」** ボタンをクリックします。

**⚠️ 日本語ユーザー名の文字化けエラー**
Windowsのユーザー名に日本語が含まれていると、コンパイル時に `boot2.o: No such file or directory` というエラーが発生します。
【解決策】
- Cドライブ直下に `C:\Temp` フォルダを作成。
- Windowsの「システム環境変数の編集」から、ユーザー環境変数の `TEMP` と `TMP` を `C:\Temp` に変更。
- Arduino IDEを再起動して書き込みを行う。

## 6. C# (PC側) プログラム実行時の注意点
1. **COMポート番号の確認**
   - USBシリアル変換器をPCに挿し、「デバイスマネージャー」の「ポート (COM と LPT)」で認識された番号（例：COM3）を確認します。
   - `Form1.cs` の `PORT_NAME = "COM3";` が一致していることを確認します。
2. **TinyUSB用のシリアル通信設定**
   - `SerialPort.DtrEnable = true;` と `RtsEnable = true;` が有効になっていないと、通信の準備ができずマイコン側がデータを取りこぼすことがあります。必ず設定をONにしてください。

## 7. AIアシスタントへの引継ぎ事項（過去のトラップと解決策まとめ）
AIにコード修正を依頼する際は、以下の事実に留意させてください。
- **物理配線の構成**：マイコンはSwitchにUSBで繋がり、PCからの操作命令は別ピン(UART0)から受け取る構成であること。
- **Serial vs Serial1**：`Serial` は使わず、必ず `Serial1` を使うこと。
- **同時押しの仕様**：2文字連続送信ではなく、C#側で1文字の専用コマンド（例:'5'）に変換して送信し、マイコン側でビット演算（`BTN_L | BTN_R`）で処理していること。
- **握り潰しエラーの可視化**：C#側の `port.Write()` などで `catch {}` と握り潰すと通信エラーに気付けないため、ログ出力や `MessageBox` を使用して開発を進めたこと。