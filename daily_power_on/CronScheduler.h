//
// CronScheduler.h
// 
// Arduino RTC-based Cron Scheduler Library
// Uses DS3231 RTC for absolute time-based scheduling
// 
// Supported Schedule Formats:
// 
// 1. Cron Expression: "minute hour day month weekday"
//    Examples:
//      "0 9 * * *"     -> Daily at 9:00
//      "30 14 * * 1"   -> Every Monday at 14:30
//      "*/5 * * * *"   -> Every 5 minutes
//      "0 */2 * * *"   -> Every 2 hours
// 
// 2. Interval (seconds): "integer"
//    Examples:
//      "60"    -> Every 60 seconds
//      "300"   -> Every 300 seconds (5 minutes)
//      "10"    -> Every 10 seconds (for testing)
//

#ifndef CRON_SCHEDULER_H
#define CRON_SCHEDULER_H

#include <Arduino.h>
#include <RTClib.h>

// Cron field structure
struct CronField {
  bool isWildcard;     // Is it *?
  bool isStep;         // Is it */n?
  int value;           // Value or step value
};

struct CronExpression {
  CronField minute;    // 0-59
  CronField hour;      // 0-23
  CronField day;       // 1-31
  CronField month;     // 1-12
  CronField weekday;   // 0-6 (0=Sunday)
};

// Schedule mode
enum ScheduleMode {
  MODE_CRON,      // Cron expression mode
  MODE_INTERVAL   // Second interval mode
};

class CronScheduler {
public:
  CronScheduler();
  
  // Initialize RTC and schedule
  bool init(const char* schedule);
  
  // Setup RTC time from serial input
  bool setupRTCTime();
  
  // Check if RTC needs time setup
  bool needsTimeSetup();
  
  // Check if should execute now
  bool shouldExecute(unsigned long currentMillis);
  
  // Get next execution time as string (for debugging)
  void getNextExecutionTime(char* buffer, size_t bufferSize);
  
  // Print debug information
  void printDebugInfo();

private:
  RTC_DS3231 _rtc;
  ScheduleMode _mode;
  CronExpression _cron;
  unsigned long _intervalSeconds;     // For interval mode
  unsigned long _nextExecutionMillis;
  unsigned long _lastCheckMillis;
  bool _initialized;
  bool _rtcAvailable;
  bool _timeIsSet;
  
  // Initialize schedule string
  bool initializeSchedule(const char* schedule);
  
  // Parse Cron expression
  bool parseCronExpression(const char* cronStr);
  
  // Parse Cron field (with step value support)
  bool parseCronField(const char* fieldStr, CronField& field, int minVal, int maxVal);
  
  // Parse interval seconds
  bool parseIntervalSeconds(const char* intervalStr);
  
  // Parse datetime string "YYYY-MM-DD HH:MM:SS"
  bool parseDateTime(const char* dateTimeStr, DateTime& dt);
  
  // Check if DateTime matches Cron expression
  bool matchesCron(const DateTime& dt);
  
  // Check if Cron field matches value (with step value support)
  bool matchesCronField(const CronField& field, int value);
  
  // Print Cron field (for debugging)
  void printCronField(const CronField& field);
};

#endif // CRON_SCHEDULER_H
