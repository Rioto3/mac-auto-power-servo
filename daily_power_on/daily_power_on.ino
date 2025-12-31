/*
 * Mac Auto Power Servo - Daily Power On (Cron Version)
 * 
 * Cron式または秒間隔でスケジュール管理するMacBook電源自動ON装置
 * 
 * 接続:
 * - サーボ信号線: A1ピン
 * - サーボVCC: 5V
 * - サーボGND: GND
 * 
 * 使い方:
 * 1. UPLOAD_DATETIME に書き込み時のUTC日時を設定
 * 2. SCHEDULE に実行スケジュールを設定
 * 3. Arduino にアップロード
 * 
 * スケジュール設定方法:
 * 
 * A) Cron式（分単位）
 *    フォーマット: "分 時 日 月 曜日"
 *    例:
 *      "0 9 * * *"     → 毎日9:00 UTC
 *      "30 14 * * *"   → 毎日14:30 UTC
 *      "*/5 * * * *"   → 5分ごと
 *      "0 */2 * * *"   → 2時間ごと
 *      "0 6 1 * *"     → 毎月1日6:00 UTC
 *      "0 0 * * 1"     → 毎週月曜0:00 UTC
 * 
 * B) 秒間隔（整数のみ）
 *    例:
 *      "60"    → 60秒ごと
 *      "300"   → 300秒（5分）ごと
 *      "10"    → 10秒ごと（テスト用）
 */

#include <Servo.h>
#include "CronScheduler.h"

// ========================================
// ユーザー設定エリア
// ========================================

// プログラム書き込み時のUTC日時
// フォーマット: "YYYY-MM-DD HH:MM:SS"
const char* UPLOAD_DATETIME = "2025-12-31 15:30:00";

// 実行スケジュール
// Cron式: "分 時 日 月 曜日"  または  秒間隔: "整数"
const char* SCHEDULE = "0 9 * * *";  // 毎日9:00 UTC
// const char* SCHEDULE = "*/5 * * * *";  // 5分ごと（テスト用）
// const char* SCHEDULE = "10";  // 10秒ごと（テスト用）

// サーボ設定
const int SERVO_PIN = A1;           // サーボ接続ピン
const int POS_REST = 0;             // 待機位置（度）
const int POS_PRESS = 90;           // 押下位置（度）
const int PRESS_DURATION = 500;     // ボタン押下時間（ミリ秒）
const int RETURN_DURATION = 500;    // 待機位置に戻る際の待機時間（ミリ秒）

// デバッグ設定
const bool DEBUG_MODE = true;       // シリアル出力の有効/無効
const int STATUS_INTERVAL = 10000;  // ステータス表示間隔（ミリ秒）

// ========================================
// グローバル変数
// ========================================

Servo powerButtonServo;
CronScheduler scheduler;
unsigned long lastStatusTime = 0;

// ========================================
// セットアップ
// ========================================

void setup() {
  // シリアル通信初期化
  if (DEBUG_MODE) {
    Serial.begin(9600);
    while (!Serial && millis() < 3000);  // シリアル接続待機（最大3秒）
    
    Serial.println(F("\n\n"));
    Serial.println(F("========================================"));
    Serial.println(F("  Mac Auto Power Servo - Cron Version"));
    Serial.println(F("========================================"));
  }
  
  // サーボ初期化
  powerButtonServo.attach(SERVO_PIN);
  powerButtonServo.write(POS_REST);
  
  if (DEBUG_MODE) {
    Serial.print(F("Servo initialized at pin A"));
    Serial.println(SERVO_PIN - A0);
    Serial.println(F("Position: REST"));
  }
  
  delay(1000);  // サーボ安定化待ち
  
  // スケジューラ初期化
  if (DEBUG_MODE) {
    Serial.println(F("\nInitializing Scheduler..."));
  }
  
  if (!scheduler.init(UPLOAD_DATETIME, SCHEDULE)) {
    if (DEBUG_MODE) {
      Serial.println(F("\n*** ERROR: Scheduler initialization failed ***"));
      Serial.println(F("Please check UPLOAD_DATETIME and SCHEDULE"));
    }
    while (1);  // エラーで停止
  }
  
  // デバッグ情報表示
  if (DEBUG_MODE) {
    scheduler.printDebugInfo();
    Serial.println(F("System ready. Waiting for scheduled time...\n"));
  }
}

// ========================================
// メインループ
// ========================================

void loop() {
  // スケジュール判定
  if (scheduleManager()) {
    servoExecute();
  }
  
  // 定期ステータス表示
  if (DEBUG_MODE && millis() - lastStatusTime >= STATUS_INTERVAL) {
    printStatus();
    lastStatusTime = millis();
  }
}

// ========================================
// スケジュール管理
// ========================================

/*
 * スケジュール判定
 * 
 * CronSchedulerを使用して実行タイミングを判定
 * 
 * Cronモード:
 * - 初回: 次のCron一致時刻
 * - 以降: 24時間ごと
 * 
 * 秒間隔モード:
 * - 起動直後に実行
 * - 以降: 指定秒数ごと
 * 
 * @return true: 実行タイミング, false: 待機
 */
bool scheduleManager() {
  return scheduler.shouldExecute(millis());
}

// ========================================
// サーボ実行
// ========================================

/*
 * サーボモーター動作実行
 * 
 * 動作シーケンス:
 * 1. 待機位置 → 押下位置（ボタンを押す）
 * 2. 押下時間だけ待機
 * 3. 押下位置 → 待機位置（元に戻る）
 */
void servoExecute() {
  if (DEBUG_MODE) {
    Serial.println(F("\n========================================"));
    Serial.println(F("[SERVO] Execution started"));
    Serial.print(F("[SERVO] Uptime: "));
    printUptime();
  }
  
  // ステップ1: ボタンを押す
  if (DEBUG_MODE) {
    Serial.print(F("[SERVO] Moving to PRESS position ("));
    Serial.print(POS_PRESS);
    Serial.println(F("°)"));
  }
  powerButtonServo.write(POS_PRESS);
  delay(PRESS_DURATION);
  
  // ステップ2: 元の位置に戻る
  if (DEBUG_MODE) {
    Serial.print(F("[SERVO] Moving to REST position ("));
    Serial.print(POS_REST);
    Serial.println(F("°)"));
  }
  powerButtonServo.write(POS_REST);
  delay(RETURN_DURATION);
  
  if (DEBUG_MODE) {
    Serial.println(F("[SERVO] Execution completed"));
    Serial.println(F("========================================\n"));
  }
}

// ========================================
// ステータス表示
// ========================================

/*
 * 現在のステータスを表示
 * 次回実行までの残り時間を出力
 */
void printStatus() {
  char buffer[32];
  scheduler.getNextExecutionTime(buffer, sizeof(buffer));
  
  Serial.print(F("[STATUS] Next execution in: "));
  Serial.println(buffer);
}

/*
 * 起動時間を表示
 */
void printUptime() {
  unsigned long totalSeconds = millis() / 1000;
  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  
  if (days > 0) {
    Serial.print(days);
    Serial.print(F("d "));
  }
  
  if (hours < 10) Serial.print(F("0"));
  Serial.print(hours);
  Serial.print(F(":"));
  if (minutes < 10) Serial.print(F("0"));
  Serial.print(minutes);
  Serial.print(F(":"));
  if (seconds < 10) Serial.print(F("0"));
  Serial.println(seconds);
}
