/*
 * CronScheduler.cpp
 * 
 * Cronスケジューラの実装
 */

#include "CronScheduler.h"

CronScheduler::CronScheduler() 
  : _uploadMillis(0), _nextExecutionMillis(0), _initialized(false), _firstExecutionDone(false) {
  memset(&_cron, 0, sizeof(CronExpression));
  memset(&_uploadTime, 0, sizeof(DateTime));
}

bool CronScheduler::init(const char* uploadDateTime, const char* cronSchedule) {
  // 日時のパース
  if (!parseDateTime(uploadDateTime)) {
    Serial.println(F("[CRON] ERROR: Failed to parse upload datetime"));
    return false;
  }
  
  // Cron式のパース
  if (!parseCronExpression(cronSchedule)) {
    Serial.println(F("[CRON] ERROR: Failed to parse cron expression"));
    return false;
  }
  
  // 現在のmillis()を記録
  _uploadMillis = millis();
  
  // 次回実行時刻を計算
  calculateNextExecution();
  
  _initialized = true;
  return true;
}

bool CronScheduler::parseDateTime(const char* dateTimeStr) {
  // フォーマット: "YYYY-MM-DD HH:MM:SS"
  int year, month, day, hour, minute, second;
  
  int parsed = sscanf(dateTimeStr, "%d-%d-%d %d:%d:%d", 
                      &year, &month, &day, &hour, &minute, &second);
  
  if (parsed != 6) {
    return false;
  }
  
  _uploadTime.year = year;
  _uploadTime.month = month;
  _uploadTime.day = day;
  _uploadTime.hour = hour;
  _uploadTime.minute = minute;
  _uploadTime.second = second;
  _uploadTime.weekday = calculateWeekday(year, month, day);
  
  return isValidDateTime(_uploadTime);
}

bool CronScheduler::parseCronExpression(const char* cronStr) {
  // フォーマット: "分 時 日 月 曜日"
  char minute[10], hour[10], day[10], month[10], weekday[10];
  
  int parsed = sscanf(cronStr, "%s %s %s %s %s", minute, hour, day, month, weekday);
  
  if (parsed != 5) {
    return false;
  }
  
  // 各フィールドをパース（* は -1）
  _cron.minute = (strcmp(minute, "*") == 0) ? -1 : atoi(minute);
  _cron.hour = (strcmp(hour, "*") == 0) ? -1 : atoi(hour);
  _cron.day = (strcmp(day, "*") == 0) ? -1 : atoi(day);
  _cron.month = (strcmp(month, "*") == 0) ? -1 : atoi(month);
  _cron.weekday = (strcmp(weekday, "*") == 0) ? -1 : atoi(weekday);
  
  // 範囲チェック
  if (_cron.minute != -1 && (_cron.minute < 0 || _cron.minute > 59)) return false;
  if (_cron.hour != -1 && (_cron.hour < 0 || _cron.hour > 23)) return false;
  if (_cron.day != -1 && (_cron.day < 1 || _cron.day > 31)) return false;
  if (_cron.month != -1 && (_cron.month < 1 || _cron.month > 12)) return false;
  if (_cron.weekday != -1 && (_cron.weekday < 0 || _cron.weekday > 6)) return false;
  
  return true;
}

void CronScheduler::calculateNextExecution() {
  // 書き込み時刻から次のCron一致時刻を検索
  DateTime nextTime = _uploadTime;
  
  // 1分刻みで最大1年先まで検索（525600分）
  for (long i = 1; i < 525600; i++) {
    addSeconds(nextTime, 60);  // 1分進める
    
    if (matchesCron(nextTime)) {
      // 一致する時刻が見つかった
      long secondsDiff = getSecondsDifference(_uploadTime, nextTime);
      _nextExecutionMillis = _uploadMillis + (secondsDiff * 1000UL);
      return;
    }
  }
  
  // 見つからなかった場合（エラー）
  Serial.println(F("[CRON] ERROR: Could not find next execution time"));
  _nextExecutionMillis = _uploadMillis + 86400000UL;  // とりあえず24時間後
}

bool CronScheduler::matchesCron(const DateTime& dt) {
  if (_cron.minute != -1 && dt.minute != _cron.minute) return false;
  if (_cron.hour != -1 && dt.hour != _cron.hour) return false;
  if (_cron.day != -1 && dt.day != _cron.day) return false;
  if (_cron.month != -1 && dt.month != _cron.month) return false;
  if (_cron.weekday != -1 && dt.weekday != _cron.weekday) return false;
  
  // 秒は00秒のみ（Cronは分単位）
  return (dt.second == 0);
}

bool CronScheduler::shouldExecute(unsigned long currentMillis) {
  if (!_initialized) return false;
  
  // 初回実行チェック
  if (!_firstExecutionDone && currentMillis >= _nextExecutionMillis) {
    _firstExecutionDone = true;
    
    // 次回実行時刻を24時間後に設定
    _nextExecutionMillis = currentMillis + 86400000UL;  // 24時間 = 86400秒
    
    return true;
  }
  
  // 2回目以降: 24時間ごと
  if (_firstExecutionDone && currentMillis >= _nextExecutionMillis) {
    _nextExecutionMillis = currentMillis + 86400000UL;
    return true;
  }
  
  return false;
}

unsigned long CronScheduler::getNextExecutionDelay() {
  if (!_initialized) return 0;
  
  unsigned long currentMillis = millis();
  if (currentMillis >= _nextExecutionMillis) {
    return 0;  // すでに実行タイミング
  }
  
  return _nextExecutionMillis - currentMillis;
}

void CronScheduler::getNextExecutionTime(char* buffer, size_t bufferSize) {
  if (!_initialized) {
    snprintf(buffer, bufferSize, "Not initialized");
    return;
  }
  
  unsigned long delayMs = getNextExecutionDelay();
  unsigned long delaySec = delayMs / 1000;
  
  unsigned long days = delaySec / 86400;
  unsigned long hours = (delaySec % 86400) / 3600;
  unsigned long minutes = (delaySec % 3600) / 60;
  unsigned long seconds = delaySec % 60;
  
  if (days > 0) {
    snprintf(buffer, bufferSize, "%lud %02lu:%02lu:%02lu", 
             days, hours, minutes, seconds);
  } else {
    snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", 
             hours, minutes, seconds);
  }
}

void CronScheduler::printDebugInfo() {
  Serial.println(F("\n========================================"));
  Serial.println(F("[CRON] Scheduler Debug Info"));
  Serial.println(F("========================================"));
  
  // 書き込み日時
  Serial.print(F("Upload DateTime: "));
  Serial.print(_uploadTime.year);
  Serial.print(F("-"));
  if (_uploadTime.month < 10) Serial.print(F("0"));
  Serial.print(_uploadTime.month);
  Serial.print(F("-"));
  if (_uploadTime.day < 10) Serial.print(F("0"));
  Serial.print(_uploadTime.day);
  Serial.print(F(" "));
  if (_uploadTime.hour < 10) Serial.print(F("0"));
  Serial.print(_uploadTime.hour);
  Serial.print(F(":"));
  if (_uploadTime.minute < 10) Serial.print(F("0"));
  Serial.print(_uploadTime.minute);
  Serial.print(F(":"));
  if (_uploadTime.second < 10) Serial.print(F("0"));
  Serial.println(_uploadTime.second);
  
  // Cron式
  Serial.print(F("Cron Expression: "));
  if (_cron.minute == -1) Serial.print(F("*")); else Serial.print(_cron.minute);
  Serial.print(F(" "));
  if (_cron.hour == -1) Serial.print(F("*")); else Serial.print(_cron.hour);
  Serial.print(F(" "));
  if (_cron.day == -1) Serial.print(F("*")); else Serial.print(_cron.day);
  Serial.print(F(" "));
  if (_cron.month == -1) Serial.print(F("*")); else Serial.print(_cron.month);
  Serial.print(F(" "));
  if (_cron.weekday == -1) Serial.print(F("*")); else Serial.print(_cron.weekday);
  Serial.println();
  
  // 次回実行まで
  char buffer[32];
  getNextExecutionTime(buffer, sizeof(buffer));
  Serial.print(F("Next Execution In: "));
  Serial.println(buffer);
  
  Serial.println(F("========================================\n"));
}

// ========================================
// ユーティリティ関数
// ========================================

bool CronScheduler::isValidDateTime(const DateTime& dt) {
  if (dt.year < 2000 || dt.year > 2100) return false;
  if (dt.month < 1 || dt.month > 12) return false;
  if (dt.day < 1 || dt.day > getDaysInMonth(dt.year, dt.month)) return false;
  if (dt.hour < 0 || dt.hour > 23) return false;
  if (dt.minute < 0 || dt.minute > 59) return false;
  if (dt.second < 0 || dt.second > 59) return false;
  return true;
}

int CronScheduler::calculateWeekday(int year, int month, int day) {
  // ツェラーの公式
  if (month < 3) {
    month += 12;
    year--;
  }
  
  int k = year % 100;
  int j = year / 100;
  
  int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
  
  // 土曜=0を日曜=0に変換
  return (h + 6) % 7;
}

bool CronScheduler::isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int CronScheduler::getDaysInMonth(int year, int month) {
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  
  return days[month - 1];
}

long CronScheduler::getSecondsDifference(const DateTime& from, const DateTime& to) {
  // 簡易実装: 年月日時分秒から秒数を概算
  // より正確な実装が必要な場合はUnixタイムスタンプ計算を使用
  
  long fromSeconds = (long)from.second + 
                     (long)from.minute * 60L + 
                     (long)from.hour * 3600L +
                     (long)from.day * 86400L;
  
  long toSeconds = (long)to.second + 
                   (long)to.minute * 60L + 
                   (long)to.hour * 3600L +
                   (long)to.day * 86400L;
  
  // 月と年の差を日数に変換（概算）
  int daysDiff = 0;
  
  // 年の差
  for (int y = from.year; y < to.year; y++) {
    daysDiff += isLeapYear(y) ? 366 : 365;
  }
  
  // 月の差
  for (int m = from.month; m < to.month; m++) {
    daysDiff += getDaysInMonth(from.year, m);
  }
  
  return toSeconds - fromSeconds + (daysDiff * 86400L);
}

void CronScheduler::addSeconds(DateTime& dt, long seconds) {
  dt.second += seconds;
  
  // 秒のオーバーフロー処理
  while (dt.second >= 60) {
    dt.second -= 60;
    dt.minute++;
  }
  
  // 分のオーバーフロー処理
  while (dt.minute >= 60) {
    dt.minute -= 60;
    dt.hour++;
  }
  
  // 時のオーバーフロー処理
  while (dt.hour >= 24) {
    dt.hour -= 24;
    dt.day++;
    dt.weekday = (dt.weekday + 1) % 7;
  }
  
  // 日のオーバーフロー処理
  int daysInMonth = getDaysInMonth(dt.year, dt.month);
  while (dt.day > daysInMonth) {
    dt.day -= daysInMonth;
    dt.month++;
    
    if (dt.month > 12) {
      dt.month = 1;
      dt.year++;
    }
    
    daysInMonth = getDaysInMonth(dt.year, dt.month);
  }
}
