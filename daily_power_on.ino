#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>

/*
  =====================================
  設定エリア
  =====================================
*/

// ===== テスト用（最優先） =====
// 0 にすると無効
// 例: 3 → 3秒ごとに実行
#define TEST_CRON 3

// ===== 本番 Cron（UTC） =====
struct CronTime {
  int hour;
  int minute;
};

// 複数定義可能
const CronTime CRON_TIMES[] = {
  {9, 0},     // 09:00
  {21, 30},   // 21:30
};
const int CRON_COUNT = sizeof(CRON_TIMES) / sizeof(CRON_TIMES[0]);

// ===== サーボ設定 =====
const int SERVO_PIN      = A1;
const int POS_REST       = 0;
const int POS_PRESS      = 20;
const int PRESS_DURATION = 500;
const int RETURN_DELAY   = 500;

// デバッグ
const bool DEBUG = true;

/*
  =====================================
  グローバル
  =====================================
*/

RTC_DS3231 rtc;
Servo servo;

unsigned long lastTestMillis = 0;
int lastExecutedDay = -1;

/*
  =====================================
  セットアップ
  =====================================
*/

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 3000);

  Serial.println(F("\n=== Mac Auto Power (RTC + Test Cron) ==="));

  servo.attach(SERVO_PIN);
  servo.write(POS_REST);

#if TEST_CRON > 0
  Serial.print(F("TEST MODE: "));
  Serial.print(TEST_CRON);
  Serial.println(F(" sec interval"));
#else
  Wire.begin();

  if (!rtc.begin()) {
    Serial.println(F("ERROR: DS3231 not found"));
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power. Setting compile time."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  printDateTime(rtc.now());
#endif

  Serial.println(F("System ready."));
}

/*
  =====================================
  ループ
  =====================================
*/

void loop() {

#if TEST_CRON > 0
  // ===== テスト優先 =====
  if (millis() - lastTestMillis >= TEST_CRON * 1000UL) {
    lastTestMillis = millis();
    executeServo();
  }
#else
  // ===== 本番 Cron =====
  DateTime now = rtc.now();

  if (shouldExecuteCron(now)) {
    executeServo();
    lastExecutedDay = now.day();
  }
#endif

  delay(200);
}

/*
  =====================================
  Cron 判定
  =====================================
*/

bool shouldExecuteCron(const DateTime& now) {
  // 1日1回制御
  if (lastExecutedDay == now.day()) return false;

  for (int i = 0; i < CRON_COUNT; i++) {
    if (now.hour() == CRON_TIMES[i].hour &&
        now.minute() == CRON_TIMES[i].minute) {

      if (DEBUG) {
        Serial.println(F("\n[CRON MATCH]"));
        printDateTime(now);
      }

      return true;
    }
  }
  return false;
}

/*
  =====================================
  サーボ実行
  =====================================
*/

void executeServo() {
  Serial.println(F("[SERVO] Press"));

  servo.write(POS_PRESS);
  delay(PRESS_DURATION);

  servo.write(POS_REST);
  delay(RETURN_DELAY);

  Serial.println(F("[SERVO] Done"));
}

/*
  =====================================
  ユーティリティ
  =====================================
*/

void printDateTime(const DateTime& dt) {
  Serial.print(dt.year()); Serial.print("-");
  if (dt.month() < 10) Serial.print("0");
  Serial.print(dt.month()); Serial.print("-");
  if (dt.day() < 10) Serial.print("0");
  Serial.print(dt.day()); Serial.print(" ");

  if (dt.hour() < 10) Serial.print("0");
  Serial.print(dt.hour()); Serial.print(":");
  if (dt.minute() < 10) Serial.print("0");
  Serial.print(dt.minute()); Serial.print(":");
  if (dt.second() < 10) Serial.print("0");
  Serial.println(dt.second());
}
