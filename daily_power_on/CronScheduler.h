/*
 * CronScheduler.h
 * 
 * Arduino用の軽量Cronスケジューラライブラリ
 * RTCなしで書き込み時刻ベースのスケジューリングを実現
 * 
 * 対応スケジュールフォーマット:
 * 
 * 1. Cron式: "分 時 日 月 曜日"
 *    例: "0 9 * * *" : 毎日9:00
 *        "30 14 * * 1" : 毎週月曜14:30
 *        "*/5 * * * *" : 5分ごと
 *        "0 */2 * * *" : 2時間ごと
 * 
 * 2. 秒間隔: "整数"
 *    例: "60" : 60秒ごと
 *        "300" : 300秒（5分）ごと
 *        "10" : 10秒ごと（テスト用）
 */

#ifndef CRON_SCHEDULER_H
#define CRON_SCHEDULER_H

#include <Arduino.h>

// Cron式の各フィールド
struct CronField {
  bool isWildcard;     // * かどうか
  bool isStep;         // */n かどうか
  int value;           // 値 または ステップ値
};

struct CronExpression {
  CronField minute;    // 0-59
  CronField hour;      // 0-23
  CronField day;       // 1-31
  CronField month;     // 1-12
  CronField weekday;   // 0-6 (0=日曜)
};

// 日時構造体
struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  int weekday;  // 0=日曜
};

// スケジュールモード
enum ScheduleMode {
  MODE_CRON,      // Cron式モード
  MODE_INTERVAL   // 秒間隔モード
};

class CronScheduler {
public:
  CronScheduler();
  
  // 初期化: 書き込み日時とスケジュール文字列を設定
  bool init(const char* uploadDateTime, const char* schedule);
  
  // 次回実行までのミリ秒数を取得
  unsigned long getNextExecutionDelay();
  
  // スケジュールチェック（現在時刻がスケジュールに合致するか）
  bool shouldExecute(unsigned long currentMillis);
  
  // 次回実行予定時刻を文字列で取得（デバッグ用）
  void getNextExecutionTime(char* buffer, size_t bufferSize);
  
  // デバッグ情報を出力
  void printDebugInfo();

private:
  ScheduleMode _mode;
  CronExpression _cron;
  DateTime _uploadTime;
  unsigned long _intervalSeconds;     // 秒間隔モード用
  unsigned long _nextExecutionMillis;
  unsigned long _uploadMillis;
  bool _initialized;
  bool _firstExecutionDone;
  
  // スケジュール文字列の判定と初期化
  bool initializeSchedule(const char* schedule);
  
  // Cron式のパース
  bool parseCronExpression(const char* cronStr);
  
  // Cronフィールドのパース（ステップ値対応）
  bool parseCronField(const char* fieldStr, CronField& field, int minVal, int maxVal);
  
  // 秒間隔のパース
  bool parseIntervalSeconds(const char* intervalStr);
  
  // 日時文字列のパース "YYYY-MM-DD HH:MM:SS"
  bool parseDateTime(const char* dateTimeStr);
  
  // 次回実行時刻を計算（Cronモード）
  void calculateNextExecutionCron();
  
  // 次回実行時刻を計算（間隔モード）
  void calculateNextExecutionInterval();
  
  // 日時が有効かチェック
  bool isValidDateTime(const DateTime& dt);
  
  // 曜日を計算（ツェラーの公式）
  int calculateWeekday(int year, int month, int day);
  
  // うるう年判定
  bool isLeapYear(int year);
  
  // 月の日数を取得
  int getDaysInMonth(int year, int month);
  
  // DateTime同士の秒数差を計算
  long getSecondsDifference(const DateTime& from, const DateTime& to);
  
  // DateTimeに秒数を加算
  void addSeconds(DateTime& dt, long seconds);
  
  // 次のCron一致時刻を検索
  bool findNextCronMatch(DateTime& dt);
  
  // DateTimeがCron式に合致するかチェック
  bool matchesCron(const DateTime& dt);
  
  // Cronフィールドが値に合致するかチェック（ステップ値対応）
  bool matchesCronField(const CronField& field, int value);
};

#endif // CRON_SCHEDULER_H
