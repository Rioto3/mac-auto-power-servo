# Mac Auto Power Servo

Arduino + サーボモーターを使って、MacBookの電源ボタンを物理的に押すプロジェクト

## 概要

Macをサーバとして使用する際、電源が切れた後に自動起動できない問題を解決するため、Arduinoとサーボモーターを使って物理的に電源ボタンを押す仕組みを構築します。

## 目的

- 24時間ごとに自動でMacBookの電源をONにする
- 停電や予期せぬシャットダウン後の自動復旧を可能にする

## 必要な部品

### 現在の構成

| 部品名 | 型番/仕様 | 数量 |
|--------|-----------|------|
| マイコンボード | Arduino Uno | 1 |
| サーボモーター | SG90 | 1 |
| ジャンパーワイヤー | オス-メス | 3本 |
| USBケーブル | Type-B (Arduino給電用) | 1 |

### 将来の拡張（オプション）

| 部品名 | 型番/仕様 | 用途 |
|--------|-----------|------|
| RTCモジュール | DS3231 | 絶対時刻指定（Cron式対応）|
| バックアップ電池 | CR2032 | RTC用バッテリー |

## 配線図

### SG90サーボモーターの接続

```
Arduino Uno          SG90サーボモーター
├─ 5V     ─────────  VCC (赤)
├─ GND    ─────────  GND (茶/黒)
└─ A1     ─────────  Signal (オレンジ/黄)
```

### ピン配置の詳細

A1ピンはアナログピンですが、デジタル出力としても使用可能です。

```
Arduino Uno アナログピン側:
[A0] [A1] [A2] [A3] [A4] [A5]
     ^^^^
     ここに接続
```

近くの5VとGNDも併せて配線してください。

## プロジェクト構成

```
mac-auto-power-servo/
├── README.md              # このファイル
├── servo_test/
│   └── servo_test.ino    # サーボモーター動作テスト用スケッチ
└── daily_power_on/
    └── daily_power_on.ino # 24時間ごと動作する本番用スケッチ ★現在はこちら
```

## 使い方

### 1. テスト動作

サーボモーターが正常に動作するか確認します。

```bash
# Arduino IDEで servo_test/servo_test.ino を開く
# ボードとポートを選択
# アップロード
```

シリアルモニタ（9600 baud）を開くと、1秒ごとにサーボの動作状況が表示されます。

### 2. 本番動作（24時間間隔実行）

24時間ごとにMacの電源ボタンを押すプログラムです。

```bash
# Arduino IDEで daily_power_on/daily_power_on.ino を開く
# ボードとポートを選択
# アップロード
```

#### 動作仕様

- **初回実行**: Arduino起動直後に1回実行（テスト用）
- **定期実行**: 以降24時間ごとに自動実行
- **時刻基準**: UTC（協定世界時）
- **デバッグ**: シリアルモニタで次回実行までの残り時間を表示

#### シリアルモニタ出力例

```
=== Mac Auto Power Servo - Daily Power On ===
Time base: UTC
Interval: 24 hours
Servo pin: A1
========================================
Servo initialized at REST position
First execution will happen immediately...

[SCHEDULE] First run - executing now
========================================
[SERVO] Execution started
[SERVO] Uptime: 1 seconds
[SERVO] Moving to PRESS position (90°)
[SERVO] Moving to REST position (0°)
[SERVO] Execution completed
========================================

[STATUS] Next execution in: 23:59:50
[STATUS] Next execution in: 23:59:40
...
```

### 3. 設定のカスタマイズ

`daily_power_on.ino` の設定エリアで以下を変更できます：

```cpp
// サーボ角度調整
const int POS_REST = 0;             // 待機位置
const int POS_PRESS = 90;           // 押下位置

// ボタン押下時間
const int PRESS_DURATION = 500;     // 押す時間（ミリ秒）

// 実行間隔（テスト用に短縮可能）
const unsigned long INTERVAL_24H = 24UL * 60UL * 60UL * 1000UL;  // 24時間
// const unsigned long INTERVAL_24H = 60000UL;  // 1分ごと（テスト用）
```

## コード構造

可読性を重視した構造になっています：

```cpp
void loop() {
  if (scheduleManager()) {  // スケジュール判定
    servoExecute();         // サーボ実行
  }
}
```

### 主要関数

- `scheduleManager()`: 24時間ごとの実行タイミングを判定
- `servoExecute()`: サーボモーターでボタン押下動作を実行
- `printStatus()`: デバッグ情報を出力

## 動作原理

1. Arduinoが24時間間隔をカウント
2. 実行タイミングになるとサーボモーターが動作
3. 電源ボタンを押して0.5秒待機
4. サーボが元の位置に戻る
5. Macが起動

## 将来の拡張予定

### RTC（リアルタイムクロック）追加

現在は「24時間ごと」ですが、RTCモジュール（DS3231）を追加すると「毎日午前9時」のような絶対時刻指定が可能になります。

**Cron式対応の実装例**：
```cpp
// "0 9 * * *" → 毎日9:00 UTC に実行
if (now.hour() == 9 && now.minute() == 0) {
  servoExecute();
}
```

コード内に `TODO` コメントで実装例を記載しています。

### その他の拡張案

- [ ] RTC（DS3231など）追加でCron式対応
- [ ] 電源ボタン押下機構の設計・3Dプリント
- [ ] 動作ログのSDカード書き込み
- [ ] 予備電源（バッテリー）の検討
- [ ] WiFi/Bluetooth経由での設定変更

## トラブルシューティング

### サーボが動かない

- 配線を確認してください（特にGND接続）
- 5V電源が供給されているか確認
- A1ピンが正しく接続されているか確認
- サーボモーターの初期不良の可能性

### サーボが震える

- 電源容量不足の可能性があります
- USB給電ではなく、外部電源の使用を検討してください

### 24時間経っても動かない

- シリアルモニタで残り時間を確認
- Arduino が再起動していないか確認（再起動するとタイマーがリセットされます）
- テスト用に間隔を短く設定（60秒など）して動作確認

## ライセンス

MIT License

## 作成者

dyethesky
