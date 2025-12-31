/*
 * Mac Auto Power Servo - Daily Power On
 * 
 * 24時間ごとにMacBookの電源ボタンを押すプログラム
 * 
 * 接続:
 * - サーボ信号線: A1ピン
 * - サーボVCC: 5V
 * - サーボGND: GND
 * 
 * 動作:
 * - 起動直後に1回実行（テスト用）
 * - 以降24時間ごとに実行
 * 
 * TODO: RTC（DS3231等）追加時にCron式対応予定
 *       例: "0 9 * * *" で毎日9:00に実行
 */

#include <Servo.h>

// ========================================
// 設定エリア
// ========================================

// ピン設定
const int SERVO_PIN = A1;           // サーボ接続ピン（A1 = デジタル15番相当）

// サーボ角度設定
const int POS_REST = 0;             // 待機位置（度）
const int POS_PRESS = 90;           // 押下位置（度）

// 動作時間設定
const int PRESS_DURATION = 500;     // ボタン押下時間（ミリ秒）
const int RETURN_DURATION = 500;    // 待機位置に戻る際の待機時間（ミリ秒）

// スケジュール設定
const unsigned long INTERVAL_24H = 24UL * 60UL * 60UL * 1000UL;  // 24時間（ミリ秒）
// const unsigned long INTERVAL_24H = 60000UL;  // テスト用: 1分ごと

// デバッグ設定
const bool DEBUG_MODE = true;       // シリアル出力の有効/無効

// ========================================
// グローバル変数
// ========================================

Servo powerButtonServo;
unsigned long lastExecutionTime = 0;
bool firstRun = true;

// ========================================
// セットアップ
// ========================================

void setup() {
  // シリアル通信初期化
  if (DEBUG_MODE) {
    Serial.begin(9600);
    Serial.println(F("=== Mac Auto Power Servo - Daily Power On ==="));
    Serial.println(F("Time base: UTC"));
    Serial.println(F("Interval: 24 hours"));
    Serial.print(F("Servo pin: A"));
    Serial.println(SERVO_PIN - A0);
    Serial.println(F("========================================"));
  }
  
  // サーボ初期化
  powerButtonServo.attach(SERVO_PIN);
  powerButtonServo.write(POS_REST);
  
  if (DEBUG_MODE) {
    Serial.println(F("Servo initialized at REST position"));
    Serial.println(F("First execution will happen immediately..."));
  }
  
  delay(1000);  // サーボ安定化待ち
}

// ========================================
// メインループ
// ========================================

void loop() {
  if (scheduleManager()) {
    servoExecute();
  }
  
  // 定期的にステータス表示（10秒ごと）
  static unsigned long lastStatusTime = 0;
  if (DEBUG_MODE && millis() - lastStatusTime >= 10000) {
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
 * 現在の実装: 24時間間隔での実行
 * 
 * TODO: RTC追加時の実装例
 * ----------------------------------------
 * #include <RTClib.h>
 * RTC_DS3231 rtc;
 * 
 * bool scheduleManager() {
 *   DateTime now = rtc.now();
 *   
 *   // Cron式: "0 9 * * *" の例（毎日9:00 UTC）
 *   if (now.hour() == 9 && now.minute() == 0 && now.second() < 2) {
 *     // 2秒以内なら実行（重複実行を防ぐ）
 *     unsigned long currentTime = millis();
 *     if (currentTime - lastExecutionTime >= 60000) {  // 1分以上経過
 *       lastExecutionTime = currentTime;
 *       return true;
 *     }
 *   }
 *   return false;
 * }
 * ----------------------------------------
 * 
 * @return true: 実行タイミング, false: 待機
 */
bool scheduleManager() {
  unsigned long currentTime = millis();
  
  // 初回実行（起動直後）
  if (firstRun) {
    firstRun = false;
    lastExecutionTime = currentTime;
    if (DEBUG_MODE) {
      Serial.println(F("\n[SCHEDULE] First run - executing now"));
    }
    return true;
  }
  
  // 24時間経過チェック
  if (currentTime - lastExecutionTime >= INTERVAL_24H) {
    lastExecutionTime = currentTime;
    if (DEBUG_MODE) {
      Serial.println(F("\n[SCHEDULE] 24 hours elapsed - executing now"));
    }
    return true;
  }
  
  return false;
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
    Serial.println(F("========================================"));
    Serial.println(F("[SERVO] Execution started"));
    Serial.print(F("[SERVO] Uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" seconds"));
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
 * 次回実行までの残り時間などを出力
 */
void printStatus() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - lastExecutionTime;
  unsigned long remaining = INTERVAL_24H - elapsed;
  
  Serial.print(F("[STATUS] Next execution in: "));
  
  // 時間を hh:mm:ss 形式で表示
  unsigned long remainingSec = remaining / 1000;
  unsigned long hours = remainingSec / 3600;
  unsigned long minutes = (remainingSec % 3600) / 60;
  unsigned long seconds = remainingSec % 60;
  
  if (hours < 10) Serial.print(F("0"));
  Serial.print(hours);
  Serial.print(F(":"));
  if (minutes < 10) Serial.print(F("0"));
  Serial.print(minutes);
  Serial.print(F(":"));
  if (seconds < 10) Serial.print(F("0"));
  Serial.println(seconds);
}
