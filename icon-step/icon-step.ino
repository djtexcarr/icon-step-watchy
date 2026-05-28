#include <Watchy.h>
#define ARDUINO_WATCHY_V20
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#include "img1.h"
#include "img2.h"
#include "img3.h"
#include "img4.h"
#include "img5.h"
#include "img6.h"
#include "img7.h"
#include "img8.h"
#include "img9.h"
#include "img10.h"

RTC_DATA_ATTR unsigned long lastManualChange = 0;

// ===== IMAGE SETUP =====
RTC_DATA_ATTR int currentImage = 0;
const int totalImages = 10;

const unsigned char* images[] = {
 img1, img2, img3, img4, img5,
 img6, img7, img8, img9, img10
};

// ===== SETTINGS =====
const int stepGoal = 1500;

// ===== RTC DATA =====
RTC_DATA_ATTR bool weekGoal[7] = {false};
RTC_DATA_ATTR int lastDay = -1;

class SimpleWatchy : public Watchy {
 using Watchy::Watchy;

float lastVBAT = 3.7;

float getStableBattery() {
float maxVBAT = 0;
for (int i = 0; i < 5; i++) {
float v = getBatteryVoltage() * 2.0;
if (v > maxVBAT) maxVBAT = v;
delay(10);
}
return maxVBAT;
}

void handleButtonPress() override {
    RTC.read(currentTime);

    uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();

    // RIGHT buttons ONLY
    if (wakeupBit & DOWN_BTN_MASK) {
        currentImage = (currentImage + 1) % totalImages;
        lastManualChange = currentTime.Minute;
        showWatchFace(true);
        return;
    }

    if (wakeupBit & UP_BTN_MASK) {
        currentImage = (currentImage - 1 + totalImages) % totalImages;
        lastManualChange = currentTime.Minute;
        showWatchFace(true);
        return;
    }

    // EVERYTHING ELSE → default menu behavior
    Watchy::handleButtonPress();
}

void drawWatchFace() override {
display.fillScreen(GxEPD_WHITE);
display.setTextColor(GxEPD_BLACK);

int todayIndex = currentTime.Wday - 1;

// ===== STEP RESET =====
if (currentTime.Hour == 0 && currentTime.Minute == 0) {
sensor.resetStepCounter();
}

// ===== WEEK RESET =====
if (currentTime.Wday == 1 && currentTime.Hour == 0 && currentTime.Minute == 0) {
for (int i = 0; i < 7; i++) weekGoal[i] = false;
}

// ===== NEW DAY =====
if (currentTime.Day != lastDay) {
 lastDay = currentTime.Day;
}

// ===== MARK GOAL =====
if (sensor.getCounter() >= stepGoal) {
weekGoal[todayIndex] = true;
}

// ===== BATTERY =====
float VBAT = getStableBattery();
if (VBAT > 2.5) lastVBAT = VBAT;
else VBAT = lastVBAT;

// ===== LEFT WEEK =====
const char* days[7] = {"S","M","T","W","T","F","S"};

int startX = 5;
int startY = 20;
int spacing = 20;

display.setTextSize(2);

for (int i = 0; i < 7; i++) {
int y = startY + (i * spacing);

int16_t x1, y1;
uint16_t w, h;
display.getTextBounds(days[i], startX, y, &x1, &y1, &w, &h);

if (i == todayIndex) {
display.fillRect(x1 - 2, y1 - 2, w + 4, h + 4, GxEPD_BLACK);

display.setTextColor(GxEPD_WHITE);
display.setCursor(startX, y);
display.print(days[i]);
display.setCursor(startX + 1, y);
display.print(days[i]);
display.setTextColor(GxEPD_BLACK);
} else {
display.setCursor(startX, y);
display.print(days[i]);

if (weekGoal[i]) {
display.drawRect(x1 - 2, y1 - 2, w + 4, h + 4, GxEPD_BLACK);
}
}
}
// ===== RIGHT PROGRESS BAR =====
int barX = 190;
int barY = 20;
int barWidth = 6;
int barHeight = 140;

display.drawRect(barX, barY, barWidth, barHeight, GxEPD_BLACK);

float progress = (float)sensor.getCounter() / stepGoal;
if (progress > 1.0) progress = 1.0;

int fillHeight = (int)(barHeight * progress);

display.fillRect(barX + 1,
                 barY + barHeight - fillHeight,
                 barWidth - 2,
                 fillHeight,
                 GxEPD_BLACK);
// ===== TIME =====
int hour = currentTime.Hour;
bool isPM = false;

if (hour >= 12) isPM = true;
if (hour > 12) hour -= 12;
if (hour == 0) hour = 12;

char timeStr[6];
sprintf(timeStr, "%d:%02d", hour, currentTime.Minute);

display.setFont(&FreeSans18pt7b);

int16_t x1, y1;
uint16_t w, h;
display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);

int timeX = (200 - w) / 2 - 15;
int timeY = 50;

display.setCursor(timeX, timeY);
display.print(timeStr);

display.setFont(NULL);
display.setTextSize(2);
display.setCursor(timeX + w + 15, timeY - 20);
display.print(isPM ? "P" : "A");

// ===== DATE =====
char dateStr[12];
sprintf(dateStr, "%02d/%02d/%04d",
currentTime.Month,
currentTime.Day,
currentTime.Year + 1970);

display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);

int dateX = (200 - w) / 2;
int dateY = timeY + 10;

display.setCursor(dateX, dateY);
display.print(dateStr);

// ===== IMAGE =====
int autoIndex = ((currentTime.Hour * 12) + (currentTime.Minute / 5)) % totalImages;

// only auto-change when minute hits 5-min boundary AND user didn't just press
if ((currentTime.Minute % 5 == 0) && (lastManualChange != currentTime.Minute)) {
 currentImage = autoIndex;
}

display.drawBitmap(
(200 - 150) / 2,
 dateY,
images[currentImage],
150,
120,
 GxEPD_BLACK
);
// ===== BATTERY (7-LEVEL SEGMENTED - BOTTOM) =====
int bx = 145;
int by = 190;

display.drawRect(bx, by, 35, 10, GxEPD_BLACK);
display.drawRect(bx + 35, by + 2, 2, 6, GxEPD_BLACK);

int level = 0;

if (VBAT > 4.1) level = 7;
else if (VBAT > 4.05) level = 6;
else if (VBAT > 4.0) level = 5;
else if (VBAT > 3.95) level = 4;
else if (VBAT > 3.9) level = 3;
else if (VBAT > 3.85) level = 2;
else if (VBAT > 3.8) level = 1;
else level = 0;

for (int i = 0; i < 7; i++) {
    if (i < level) {
        display.fillRect(bx + 2 + (i * 4), by + 2, 3, 6, GxEPD_BLACK);
    }
}
// ===== STEPS =====
display.setFont(NULL);
display.setTextSize(2);
display.setCursor(10, 185);
display.print("Steps:");
display.print(sensor.getCounter());
}
};

// ===== GLOBAL =====
watchySettings settings;
SimpleWatchy watchy(settings);

void setup() {
watchy.init();
}

void loop() {}