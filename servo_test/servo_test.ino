/*
 * Mac Auto Power Servo - Test Sketch
 * 
 * サーボモーターの動作テスト用プログラム
 * 1秒ごとにサーボを0度→90度と動かし続けます
 * 
 * 接続:
 * - サーボ信号線: デジタルピン9
 * - サーボVCC: 5V
 * - サーボGND: GND
 */

#include <Servo.h>

// サーボオブジェクトの作成
Servo powerButtonServo;

// 定数定義
const int SERVO_PIN = 9;        // サーボ接続ピン
const int POS_REST = 0;         // 待機位置（0度）
const int POS_PRESS = 90;       // 押下位置（90度）
const int DELAY_TIME = 1000;    // 動作間隔（ミリ秒）

// 現在の位置を追跡
int currentPosition = POS_REST;

void setup() {
  // シリアル通信を開始（デバッグ用）
  Serial.begin(9600);
  Serial.println("=== Mac Auto Power Servo Test ===");
  Serial.println("Starting servo test...");
  
  // サーボをピン9に接続
  powerButtonServo.attach(SERVO_PIN);
  
  // 初期位置（0度）に移動
  powerButtonServo.write(POS_REST);
  currentPosition = POS_REST;
  
  Serial.print("Servo attached to pin ");
  Serial.println(SERVO_PIN);
  Serial.println("Test pattern: 0° -> 90° -> 0° -> 90° ...");
  Serial.println("----------------------------------");
  
  delay(1000); // 初期化待ち
}

void loop() {
  // 位置を切り替え
  if (currentPosition == POS_REST) {
    // 待機位置 → 押下位置
    currentPosition = POS_PRESS;
    powerButtonServo.write(POS_PRESS);
    Serial.print("[");
    Serial.print(millis() / 1000);
    Serial.print("s] Moving to PRESS position: ");
    Serial.print(POS_PRESS);
    Serial.println("°");
  } else {
    // 押下位置 → 待機位置
    currentPosition = POS_REST;
    powerButtonServo.write(POS_REST);
    Serial.print("[");
    Serial.print(millis() / 1000);
    Serial.print("s] Moving to REST position: ");
    Serial.print(POS_REST);
    Serial.println("°");
  }
  
  // 1秒待機
  delay(DELAY_TIME);
}
