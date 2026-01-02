/*
 * Mac Auto Power Servo - Daily Power On (RTC + Cron Version)
 * 
 * RTC-based Cron scheduler for MacBook auto power-on
 * 
 * Hardware:
 * - Servo signal: A1 pin
 * - Servo VCC: 5V
 * - Servo GND: GND
 * - RTC SDA: A4
 * - RTC SCL: A5
 * 
 * Setup:
 * 1. Set CURRENT_DATETIME to current UTC time
 * 2. Set SCHEDULE (Cron expression or interval)
 * 3. Upload to Arduino
 * 
 * Schedule Format:
 * 
 * A) Cron Expression (absolute time)
 *    Format: "minute hour day month weekday"
 *    Examples:
 *      "0 9 * * *"     -> Daily at 9:00 UTC
 *      "30 14 * * *"   -> Daily at 14:30 UTC
 *      "*/5 * * * *"   -> Every 5 minutes
 *      "0 */2 * * *"   -> Every 2 hours
 *      "0 6 1 * *"     -> 1st of month at 6:00 UTC
 *      "0 0 * * 1"     -> Every Monday at 0:00 UTC
 * 
 * B) Interval (seconds)
 *    Examples:
 *      "60"    -> Every 60 seconds
 *      "300"   -> Every 300 seconds (5 minutes)
 *      "10"    -> Every 10 seconds (for testing)
 */

#include <Servo.h>
#include "CronScheduler.h"

// ========================================
// User Configuration
// ========================================

// Current UTC datetime (set this when uploading)
// Format: "YYYY-MM-DD HH:MM:SS"
const char* CURRENT_DATETIME = "2026-01-02 10:30:00";

// Execution schedule
// Cron: "minute hour day month weekday"  OR  Interval: "seconds"
const char* SCHEDULE = "0 9 * * *";  // Daily at 9:00 UTC
// const char* SCHEDULE = "*/5 * * * *";  // Every 5 minutes (testing)
// const char* SCHEDULE = "10";  // Every 10 seconds (testing)

// Servo settings
const int SERVO_PIN = A1;           // Servo pin
const int POS_REST = 0;             // Rest position (degrees)
const int POS_PRESS = 90;           // Press position (degrees)
const int PRESS_DURATION = 500;     // Button press time (ms)
const int RETURN_DURATION = 500;    // Return wait time (ms)

// Debug settings
const bool DEBUG_MODE = true;       // Serial output enable/disable
const int STATUS_INTERVAL = 10000;  // Status display interval (ms)

// ========================================
// Global Variables
// ========================================

Servo powerButtonServo;
CronScheduler scheduler;
unsigned long lastStatusTime = 0;

// ========================================
// Setup
// ========================================

void setup() {
  // Serial init
  if (DEBUG_MODE) {
    Serial.begin(9600);
    while (!Serial && millis() < 3000);  // Wait for serial (max 3s)
    
    Serial.println(F("\n\n"));
    Serial.println(F("========================================"));
    Serial.println(F("  Mac Auto Power Servo - RTC Version"));
    Serial.println(F("========================================"));
  }
  
  // Servo init
  powerButtonServo.attach(SERVO_PIN);
  powerButtonServo.write(POS_REST);
  
  if (DEBUG_MODE) {
    Serial.print(F("Servo initialized at pin A"));
    Serial.println(SERVO_PIN - A0);
    Serial.println(F("Position: REST"));
  }
  
  delay(1000);  // Servo stabilization
  
  // Scheduler init
  if (DEBUG_MODE) {
    Serial.println(F("\nInitializing RTC Scheduler..."));
  }
  
  if (!scheduler.init(CURRENT_DATETIME, SCHEDULE)) {
    if (DEBUG_MODE) {
      Serial.println(F("\n*** ERROR: Scheduler initialization failed ***"));
      Serial.println(F("Please check:"));
      Serial.println(F("- DS3231 RTC connection (SDA->A4, SCL->A5)"));
      Serial.println(F("- CURRENT_DATETIME format"));
      Serial.println(F("- SCHEDULE format"));
    }
    while (1);  // Stop on error
  }
  
  // Debug info
  if (DEBUG_MODE) {
    scheduler.printDebugInfo();
    Serial.println(F("System ready. Monitoring schedule...\n"));
  }
}

// ========================================
// Main Loop
// ========================================

void loop() {
  // Schedule check
  if (scheduleManager()) {
    servoExecute();
  }
  
  // Status display
  if (DEBUG_MODE && millis() - lastStatusTime >= STATUS_INTERVAL) {
    printStatus();
    lastStatusTime = millis();
  }
}

// ========================================
// Schedule Manager
// ========================================

/*
 * Schedule check
 * 
 * Uses CronScheduler with RTC
 * 
 * Cron mode:
 * - Checks RTC time every second
 * - Executes when Cron expression matches
 * 
 * Interval mode:
 * - Executes immediately on first run
 * - Then executes every N seconds
 * 
 * @return true: execute now, false: wait
 */
bool scheduleManager() {
  return scheduler.shouldExecute(millis());
}

// ========================================
// Servo Execution
// ========================================

/*
 * Servo motor execution
 * 
 * Sequence:
 * 1. Rest -> Press position (push button)
 * 2. Wait for press duration
 * 3. Press -> Rest position (return)
 */
void servoExecute() {
  if (DEBUG_MODE) {
    Serial.println(F("\n========================================"));
    Serial.println(F("[SERVO] Execution started"));
    Serial.print(F("[SERVO] Uptime: "));
    printUptime();
  }
  
  // Step 1: Push button
  if (DEBUG_MODE) {
    Serial.print(F("[SERVO] Moving to PRESS position ("));
    Serial.print(POS_PRESS);
    Serial.println(F(" degrees)"));
  }
  powerButtonServo.write(POS_PRESS);
  delay(PRESS_DURATION);
  
  // Step 2: Return to rest
  if (DEBUG_MODE) {
    Serial.print(F("[SERVO] Moving to REST position ("));
    Serial.print(POS_REST);
    Serial.println(F(" degrees)"));
  }
  powerButtonServo.write(POS_REST);
  delay(RETURN_DURATION);
  
  if (DEBUG_MODE) {
    Serial.println(F("[SERVO] Execution completed"));
    Serial.println(F("========================================\n"));
  }
}

// ========================================
// Status Display
// ========================================

/*
 * Display current status
 */
void printStatus() {
  char buffer[32];
  scheduler.getNextExecutionTime(buffer, sizeof(buffer));
  
  Serial.print(F("[STATUS] "));
  Serial.println(buffer);
}

/*
 * Display uptime
 */
void printUptime() {
  unsigned long totalSeconds = millis() / 1000;
  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  
  if (days > 0) {
    Serial.print(days);
    Serial.print(F("d "));
  }
  
  if (hours < 10) Serial.print(F("0"));
  Serial.print(hours);
  Serial.print(F(":"));
  if (minutes < 10) Serial.print(F("0"));
  Serial.print(minutes);
  Serial.print(F(":"));
  if (seconds < 10) Serial.print(F("0"));
  Serial.println(seconds);
}
