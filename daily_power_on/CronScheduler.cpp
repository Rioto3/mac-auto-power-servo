/*
 * CronScheduler.cpp
 * 
 * Cron Scheduler Implementation with RTC support
 */

#include "CronScheduler.h"

CronScheduler::CronScheduler() 
  : _mode(MODE_CRON), _nextExecutionMillis(0), _lastCheckMillis(0),
    _intervalSeconds(0), _initialized(false), _rtcAvailable(false) {
  memset(&_cron, 0, sizeof(CronExpression));
}

bool CronScheduler::init(const char* currentDateTime, const char* schedule) {
  // Initialize RTC
  if (!_rtc.begin()) {
    Serial.println(F("[CRON] ERROR: Could not find RTC"));
    return false;
  }
  
  _rtcAvailable = true;
  
  // Parse and set current datetime to RTC
  DateTime dt;
  if (!parseDateTime(currentDateTime, dt)) {
    Serial.println(F("[CRON] ERROR: Failed to parse current datetime"));
    return false;
  }
  
  // Set RTC time
  _rtc.adjust(dt);
  Serial.print(F("[CRON] RTC time set to: "));
  Serial.print(dt.year());
  Serial.print(F("-"));
  Serial.print(dt.month());
  Serial.print(F("-"));
  Serial.print(dt.day());
  Serial.print(F(" "));
  Serial.print(dt.hour());
  Serial.print(F(":"));
  Serial.print(dt.minute());
  Serial.print(F(":"));
  Serial.println(dt.second());
  
  // Check if RTC lost power
  if (_rtc.lostPower()) {
    Serial.println(F("[CRON] WARNING: RTC lost power, time was reset"));
  }
  
  // Parse schedule
  if (!initializeSchedule(schedule)) {
    Serial.println(F("[CRON] ERROR: Failed to parse schedule"));
    return false;
  }
  
  _lastCheckMillis = millis();
  _initialized = true;
  
  return true;
}

bool CronScheduler::initializeSchedule(const char* schedule) {
  // Check if it contains space (Cron format)
  bool hasCronFormat = false;
  for (int i = 0; schedule[i] != '\0'; i++) {
    if (schedule[i] == ' ') {
      hasCronFormat = true;
      break;
    }
  }
  
  if (hasCronFormat) {
    // Cron expression mode
    _mode = MODE_CRON;
    return parseCronExpression(schedule);
  } else {
    // Interval mode
    _mode = MODE_INTERVAL;
    return parseIntervalSeconds(schedule);
  }
}

bool CronScheduler::parseIntervalSeconds(const char* intervalStr) {
  // Check if it's numeric only
  for (int i = 0; intervalStr[i] != '\0'; i++) {
    if (intervalStr[i] < '0' || intervalStr[i] > '9') {
      return false;
    }
  }
  
  long interval = atol(intervalStr);
  
  if (interval <= 0 || interval > 86400) {  // 0 < interval <= 24 hours
    return false;
  }
  
  _intervalSeconds = interval;
  _nextExecutionMillis = millis();  // Execute immediately on first run
  
  return true;
}

bool CronScheduler::parseDateTime(const char* dateTimeStr, DateTime& dt) {
  // Format: "YYYY-MM-DD HH:MM:SS"
  int year, month, day, hour, minute, second;
  
  int parsed = sscanf(dateTimeStr, "%d-%d-%d %d:%d:%d", 
                      &year, &month, &day, &hour, &minute, &second);
  
  if (parsed != 6) {
    return false;
  }
  
  dt = DateTime(year, month, day, hour, minute, second);
  
  return true;
}

bool CronScheduler::parseCronExpression(const char* cronStr) {
  // Format: "minute hour day month weekday"
  char minute[20], hour[20], day[20], month[20], weekday[20];
  
  int parsed = sscanf(cronStr, "%s %s %s %s %s", minute, hour, day, month, weekday);
  
  if (parsed != 5) {
    return false;
  }
  
  // Parse each field (with step value support)
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
  
  // Wildcard check
  if (strcmp(fieldStr, "*") == 0) {
    field.isWildcard = true;
    return true;
  }
  
  // Step value check (*/n format)
  if (fieldStr[0] == '*' && fieldStr[1] == '/') {
    field.isStep = true;
    field.value = atoi(fieldStr + 2);  // "*/5" -> 5
    
    if (field.value <= 0 || field.value > maxVal) {
      return false;
    }
    
    return true;
  }
  
  // Numeric check
  field.value = atoi(fieldStr);
  
  if (field.value < minVal || field.value > maxVal) {
    return false;
  }
  
  return true;
}

bool CronScheduler::matchesCron(const DateTime& dt) {
  if (!matchesCronField(_cron.minute, dt.minute())) return false;
  if (!matchesCronField(_cron.hour, dt.hour())) return false;
  if (!matchesCronField(_cron.day, dt.day())) return false;
  if (!matchesCronField(_cron.month, dt.month())) return false;
  if (!matchesCronField(_cron.weekday, dt.dayOfTheWeek())) return false;
  
  // Match on 00 seconds (Cron is minute-based)
  return (dt.second() == 0);
}

bool CronScheduler::matchesCronField(const CronField& field, int value) {
  // Wildcard always matches
  if (field.isWildcard) {
    return true;
  }
  
  // Step value
  if (field.isStep) {
    return (value % field.value) == 0;
  }
  
  // Normal value comparison
  return field.value == value;
}

bool CronScheduler::shouldExecute(unsigned long currentMillis) {
  if (!_initialized || !_rtcAvailable) return false;
  
  if (_mode == MODE_INTERVAL) {
    // Interval mode
    if (currentMillis >= _nextExecutionMillis) {
      _nextExecutionMillis = currentMillis + (_intervalSeconds * 1000UL);
      return true;
    }
    return false;
  }
  
  // Cron mode: check every second
  if (currentMillis - _lastCheckMillis >= 1000) {
    _lastCheckMillis = currentMillis;
    
    DateTime now = _rtc.now();
    
    if (matchesCron(now)) {
      // Matched! Set next check to avoid duplicate execution
      _lastCheckMillis = currentMillis + 60000;  // Skip next 60 seconds
      return true;
    }
  }
  
  return false;
}

unsigned long CronScheduler::getNextExecutionDelay() {
  if (!_initialized) return 0;
  
  if (_mode == MODE_INTERVAL) {
    unsigned long currentMillis = millis();
    if (currentMillis >= _nextExecutionMillis) {
      return 0;
    }
    return _nextExecutionMillis - currentMillis;
  }
  
  // Cron mode: estimate time until next match
  // This is approximate - just for display purposes
  return 60000;  // Show "checking every minute"
}

void CronScheduler::getNextExecutionTime(char* buffer, size_t bufferSize) {
  if (!_initialized) {
    snprintf(buffer, bufferSize, "Not initialized");
    return;
  }
  
  if (_mode == MODE_INTERVAL) {
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
  } else {
    // Cron mode
    snprintf(buffer, bufferSize, "Checking Cron match...");
  }
}

void CronScheduler::printDebugInfo() {
  Serial.println(F("\n========================================"));
  Serial.println(F("[CRON] Scheduler Debug Info"));
  Serial.println(F("========================================"));
  
  // Schedule mode
  Serial.print(F("Schedule Mode: "));
  if (_mode == MODE_CRON) {
    Serial.println(F("Cron Expression (RTC-based)"));
  } else {
    Serial.println(F("Interval (seconds)"));
  }
  
  // RTC status
  Serial.print(F("RTC Available: "));
  Serial.println(_rtcAvailable ? F("Yes") : F("No"));
  
  if (_rtcAvailable) {
    DateTime now = _rtc.now();
    Serial.print(F("Current RTC Time: "));
    Serial.print(now.year());
    Serial.print(F("-"));
    if (now.month() < 10) Serial.print(F("0"));
    Serial.print(now.month());
    Serial.print(F("-"));
    if (now.day() < 10) Serial.print(F("0"));
    Serial.print(now.day());
    Serial.print(F(" "));
    if (now.hour() < 10) Serial.print(F("0"));
    Serial.print(now.hour());
    Serial.print(F(":"));
    if (now.minute() < 10) Serial.print(F("0"));
    Serial.print(now.minute());
    Serial.print(F(":"));
    if (now.second() < 10) Serial.print(F("0"));
    Serial.println(now.second());
    
    Serial.print(F("Day of Week: "));
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    Serial.println(days[now.dayOfTheWeek()]);
  }
  
  // Schedule details
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
    
    char buffer[32];
    getNextExecutionTime(buffer, sizeof(buffer));
    Serial.print(F("Next Execution In: "));
    Serial.println(buffer);
  }
  
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
