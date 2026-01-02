#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>

// =======================
// 設定
// =======================

#define TEST_CRON 0   // 0 = 本番

struct CronTime {
  int hour;
  int minute;
};

const CronTime CRON_TIMES[] = {
  {23, 30},   // JST 08:30 (UTC)
};
const int CRON_COUNT = sizeof(CRON_TIMES) / sizeof(CRON_TIMES[0]);

const int SERVO_PIN      = A1;
const int POS_REST       = 0;
const int POS_PRESS      = 20;
const int PRESS_DURATION = 1500;
const int RETURN_DELAY   = 500;

const bool DEBUG = true;

// =======================
// グローバル
// =======================

RTC_DS3231 rtc;
Servo servo;

unsigned long lastTestMillis = 0;
unsigned long lastRtcLogMillis = 0;
int lastExecutedDay = -1;

// =======================
// setup
// =======================

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 3000);

  Serial.println(F("\n=== BOOT ==="));

  Wire.begin();

  if (!rtc.begin()) {
    Serial.println(F("[RTC] ERROR: not found"));
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println(F("[RTC] WARNING: lost power"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("[RTC] time set from compile time"));
  }

  DateTime now = rtc.now();
  Serial.print(F("[RTC] current time: "));
  printDateTime(now);

  Serial.println(F("=== READY ==="));
}


// =======================
// loop
// =======================

void loop() {
  DateTime now = rtc.now();

  if (shouldExecuteCron(now)) {
    executeServo();
    lastExecutedDay = now.day();
  }

  delay(200);
}


// =======================
// Cron 判定
// =======================

bool shouldExecuteCron(const DateTime& now) {
  if (lastExecutedDay == now.day()) {
    Serial.println(F("[CRON] skipped (already executed today)"));
    return false;
  }

  for (int i = 0; i < CRON_COUNT; i++) {

    if (now.hour() != CRON_TIMES[i].hour) {
      Serial.println(F("[CRON] hour mismatch"));
      continue;
    }

    if (now.minute() != CRON_TIMES[i].minute) {
      Serial.println(F("[CRON] minute mismatch"));
      continue;
    }

    Serial.println(F("[CRON] MATCH"));
    return true;
  }

  return false;
}

// =======================
// Servo
// =======================

void executeServo() {
  Serial.println(F("[SERVO] Press"));
  servo.write(POS_PRESS);
  delay(PRESS_DURATION);
  servo.write(POS_REST);
  delay(RETURN_DELAY);
  Serial.println(F("[SERVO] Done"));
}

// =======================
// Utils
// =======================

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
