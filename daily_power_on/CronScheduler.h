/*
 * CronScheduler.h
 * 
 * Arduino用の軽量Cronスケジューラライブラリ
 * RTCなしで書き込み時刻ベースのスケジューリングを実現
 * 
 * 対応Cron式フォーマット: "分 時 日 月 曜日"
 * 例: "0 9 * * *" → 毎日9:00
 *     "30 14 * * 1" → 毎週月曜14:30
 */

#ifndef CRON_SCHEDULER_H
#define CRON_SCHEDULER_H

#include <Arduino.h>

// Cron式の各フィールド
struct CronExpression {
  int minute;      // 0-59, -1 = *
  int hour;        // 0-23, -1 = *
  int day;         // 1-31, -1 = *
  int month;       // 1-12, -1 = *
  int weekday;     // 0-6 (0=日曜), -1 = *
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

class CronScheduler {
public:
  CronScheduler();
  
  // 初期化: 書き込み日時とCron式を設定
  bool init(const char* uploadDateTime, const char* cronSchedule);
  
  // 次回実行までのミリ秒数を取得
  unsigned long getNextExecutionDelay();
  
  // スケジュールチェック（現在時刻がCron式に合致するか）
  bool shouldExecute(unsigned long currentMillis);
  
  // 次回実行予定時刻を文字列で取得（デバッグ用）
  void getNextExecutionTime(char* buffer, size_t bufferSize);
  
  // デバッグ情報を出力
  void printDebugInfo();

private:
  CronExpression _cron;
  DateTime _uploadTime;
  unsigned long _nextExecutionMillis;
  unsigned long _uploadMillis;
  bool _initialized;
  bool _firstExecutionDone;
  
  // Cron式のパース
  bool parseCronExpression(const char* cronStr);
  
  // 日時文字列のパース "YYYY-MM-DD HH:MM:SS"
  bool parseDateTime(const char* dateTimeStr);
  
  // 次回実行時刻を計算
  void calculateNextExecution();
  
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
};

#endif // CRON_SCHEDULER_H
