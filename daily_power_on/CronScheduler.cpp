/*
 * CronScheduler.cpp
 * 
 * Cronスケジューラの実装
 */

#include "CronScheduler.h"

CronScheduler::CronScheduler() 
  : _mode(MODE_CRON), _uploadMillis(0), _nextExecutionMillis(0), 
    _intervalSeconds(0), _initialized(false), _firstExecutionDone(false) {
  memset(&_cron, 0, sizeof(CronExpression));
  memset(&_uploadTime, 0, sizeof(DateTime));
}

bool CronScheduler::init(const char* uploadDateTime, const char* schedule) {
  // 日時のパース
  if (!parseDateTime(uploadDateTime)) {
    Serial.println(F("[CRON] ERROR: Failed to parse upload datetime"));
    return false;
  }
  
  // スケジュール文字列の解析と初期化
  if (!initializeSchedule(schedule)) {
    Serial.println(F("[CRON] ERROR: Failed to parse schedule"));
    return false;
  }
  
  // 現在のmillis()を記録
  _uploadMillis = millis();
  
  // 次回実行時刻を計算
  if (_mode == MODE_CRON) {
    calculateNextExecutionCron();
  } else {
    calculateNextExecutionInterval();
  }
  
  _initialized = true;
  return true;
}

bool CronScheduler::initializeSchedule(const char* schedule) {
  // スペースを含むかチェック（Cron式の判定）
  bool hasCronFormat = false;
  for (int i = 0; schedule[i] != '\0'; i++) {
    if (schedule[i] == ' ') {
      hasCronFormat = true;
      break;
    }
  }
  
  if (hasCronFormat) {
    // Cron式モード
    _mode = MODE_CRON;
    return parseCronExpression(schedule);
  } else {
    // 秒間隔モード
    _mode = MODE_INTERVAL;
    return parseIntervalSeconds(schedule);
  }
}

bool CronScheduler::parseIntervalSeconds(const char* intervalStr) {
  // 数値のみかチェック
  for (int i = 0; intervalStr[i] != '\0'; i++) {
    if (intervalStr[i] < '0' || intervalStr[i] > '9') {
      return false;  // 数字以外が含まれている
    }
  }
  
  long interval = atol(intervalStr);
  
  if (interval <= 0 || interval > 86400) {  // 0秒 < interval <= 24時間
    return false;
  }
  
  _intervalSeconds = interval;
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
  char minute[20], hour[20], day[20], month[20], weekday[20];
  
  int parsed = sscanf(cronStr, "%s %s %s %s %s", minute, hour, day, month, weekday);
  
  if (parsed != 5) {
    return false;
  }
  
  // 各フィールドをパース（ステップ値対応）
  if (!parseCronField(minute, _cron.minute, 0, 59)) return false;
  if (!parseCronField(hour, _cron.hour, 0, 23)) return false;
  if (!parseCronField(day, _cron.day, 1, 31)) return false;
  if (!parseCronField(month, _cron.month, 1, 12)) return false;
  if (!parseCronField(weekday, _cron.weekday, 0, 6)) return false;
  
  return true;
}

bool CronScheduler::parseCronField(const char* fieldStr, CronField& field, int minVal, int maxVal) {
  field.isWildcard = false;
  field.isStep = false;
  field.value = 0;
  
  // ワイルドカードチェック
  if (strcmp(fieldStr, "*") == 0) {
    field.isWildcard = true;
    return true;
  }
  
  // ステップ値チェック（*/n 形式）
  if (fieldStr[0] == '*' && fieldStr[1] == '/') {
    field.isStep = true;
    field.value = atoi(fieldStr + 2);  // "*/5" → 5
    
    if (field.value <= 0 || field.value > maxVal) {
      return false;
    }
    
    return true;
  }
  
  // 数値チェック（n/m 形式には未対応）
  field.value = atoi(fieldStr);
  
  if (field.value < minVal || field.value > maxVal) {
    return false;
  }
  
  return true;
}

void CronScheduler::calculateNextExecutionCron() {
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

void CronScheduler::calculateNextExecutionInterval() {
  // 秒間隔モード: 起動直後に実行し、以降は指定秒数ごと
  _nextExecutionMillis = _uploadMillis;  // 即座に実行
}

bool CronScheduler::matchesCron(const DateTime& dt) {
  if (!matchesCronField(_cron.minute, dt.minute)) return false;
  if (!matchesCronField(_cron.hour, dt.hour)) return false;
  if (!matchesCronField(_cron.day, dt.day)) return false;
  if (!matchesCronField(_cron.month, dt.month)) return false;
  if (!matchesCronField(_cron.weekday, dt.weekday)) return false;
  
  // 秒は00秒のみ（Cronは分単位）
  return (dt.second == 0);
}

bool CronScheduler::matchesCronField(const CronField& field, int value) {
  // ワイルドカードは常に一致
  if (field.isWildcard) {
    return true;
  }
  
  // ステップ値の場合
  if (field.isStep) {
    return (value % field.value) == 0;
  }
  
  // 通常の値比較
  return field.value == value;
}

bool CronScheduler::shouldExecute(unsigned long currentMillis) {
  if (!_initialized) return false;
  
  if (_mode == MODE_INTERVAL) {
    // 秒間隔モード
    if (currentMillis >= _nextExecutionMillis) {
      _nextExecutionMillis = currentMillis + (_intervalSeconds * 1000UL);
      return true;
    }
    return false;
  }
  
  // Cronモード
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
  
  // スケジュールモード
  Serial.print(F("Schedule Mode: "));
  if (_mode == MODE_CRON) {
    Serial.println(F("Cron Expression"));
  } else {
    Serial.println(F("Interval (seconds)"));
  }
  
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
  
  // スケジュール詳細
  if (_mode == MODE_CRON) {
    Serial.print(F("Cron Expression: "));
    printCronField(_cron.minute);
    Serial.print(F(" "));
    printCronField(_cron.hour);
    Serial.print(F(" "));
    printCronField(_cron.day);
    Serial.print(F(" "));
    printCronField(_cron.month);
    Serial.print(F(" "));
    printCronField(_cron.weekday);
    Serial.println();
  } else {
    Serial.print(F("Interval: "));
    Serial.print(_intervalSeconds);
    Serial.println(F(" seconds"));
  }
  
  // 次回実行まで
  char buffer[32];
  getNextExecutionTime(buffer, sizeof(buffer));
  Serial.print(F("Next Execution In: "));
  Serial.println(buffer);
  
  Serial.println(F("========================================\n"));
}

void CronScheduler::printCronField(const CronField& field) {
  if (field.isWildcard) {
    Serial.print(F("*"));
  } else if (field.isStep) {
    Serial.print(F("*/"));
    Serial.print(field.value);
  } else {
    Serial.print(field.value);
  }
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
