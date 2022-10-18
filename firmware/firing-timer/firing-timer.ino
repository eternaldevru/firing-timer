/*
  * firing-timer v.1.0
  * Copyright © 2022 Chernov Maksim
  * Released under the MIT License.
  * GitHub - https://github.com/eternaldevru
  * Telegram - https://t.me/eternaldev_ru
  * Website - https://eternaldev.ru 
*/

#include <LiquidCrystal_I2C.h>

#define pwrLED 3
#define mainFilamentLED 4
#define backupFilamentLED 5
#define modeBtn 6
#define startBtn 7
#define relay 8
#define buzzer 9

bool mode;
bool start;
bool _stop;
bool isFiring;
bool finish;
bool pwrLEDState;

bool flagModeBtn;
uint32_t modeBtnTimer;
bool modeBtnState;

bool flagStartBtn;
uint32_t startBtnTimer;
bool startBtnState;

uint32_t sec = 1;
uint32_t _min;
uint32_t previousMillis = 0;
int16_t interval = 1000;

uint32_t totalSec = 1;
uint32_t totalMin = 60;
uint32_t totalPreviousMillis = 0;

uint32_t blinkPreviousMillis = 0;

uint8_t _step = 1;

int16_t beepFrequency[3] = {1700, 1800, 1900};

LiquidCrystal_I2C lcd(0x3F, 20, 4);

void setup()
{

  pinMode(pwrLED, OUTPUT);
  pinMode(mainFilamentLED, OUTPUT);
  pinMode(backupFilamentLED, OUTPUT);
  pinMode(modeBtn, INPUT);
  pinMode(startBtn, INPUT);
  pinMode(relay, OUTPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(pwrLED, HIGH);
  pwrLEDState = true;

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(4, 1);
  lcd.print("Firing timer\0");
  lcd.setCursor(8, 2);
  lcd.print("v.1.0\0");
  delay(2000);

  for (uint8_t i = 0; i < 3; i++) {
    tone(buzzer, beepFrequency[i]);
    delay(150);
    noTone(buzzer);
    delay(5);
  }

  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("SHCH-1 VSZHD\0");
  delay(2000);
  lcd.clear();

  if (digitalRead(modeBtn)) {
    mode = true;
    digitalWrite(mainFilamentLED, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Mode: main\0");
  }
  else {
    digitalWrite(backupFilamentLED, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Mode: backup\0");
  }

  printInstructions();

}

void loop()
{

  modeBtnState = digitalRead(modeBtn);

  if (modeBtnState && !flagModeBtn && !start && millis() - modeBtnState > 150) {
   
    flagModeBtn = true;
    modeBtnTimer = millis();
    digitalWrite(mainFilamentLED, HIGH);
    digitalWrite(backupFilamentLED, LOW);
    mode = true;
    
    beep();

    lcd.setCursor(0, 0);
    lcd.print("Mode: main\0");
    lcd.print("\32\32\0");

  }

  if (!modeBtnState && flagModeBtn && !start && millis() - modeBtnState > 150) {
    
    flagModeBtn = false;
    modeBtnTimer = millis();
    digitalWrite(mainFilamentLED, LOW);
    digitalWrite(backupFilamentLED, HIGH);
    mode = false;

    beep();

    lcd.setCursor(0, 0);
    lcd.print("Mode: backup\0");

  }

  startBtnState = digitalRead(startBtn);

  if (startBtnState && !flagStartBtn && millis() - startBtnState > 150) {
    
    flagStartBtn = true;
    startBtnTimer = millis();
    beep();

  }

  if (!startBtnState && flagStartBtn && millis() - startBtnState > 150) {
    
    if (start) {

      flagStartBtn = false;
      startBtnTimer = millis();

      digitalWrite(pwrLED, HIGH);
      
      lcd.clear();
      lcd.setCursor(6, 1);
      lcd.print("Reset...\0");
      delay(1000);     

      reset();
      printInstructions();
      checkMode();

    }
    else {

      flagStartBtn = false;
      start = true;

      if (mode) {
        _min = 5;
      }
      else {
        _min = 1;
      }

      startBtnTimer = millis();

      clearStrings();
      lcd.setCursor(0, 1);
      lcd.print("Firing:\0");
      if (mode) {
        lcd.setCursor(0, 2);
        lcd.print("Step: 1/10\0");
        lcd.setCursor(0, 3);
        lcd.print("Time left:\0");
      }
      
      isFiring = true;

    }
  }

  if (start) {

    if(!mode) {

      timer();
      pwrBlink();
      digitalWrite(relay, HIGH);
            
      if (_stop && isFiring) {
        
        reset();
        finishScreen();
        printInstructions();
        checkMode();

      } 
      
    }
    else {
      if (_step <= 10) {
        
        timer();
        totalTimer();
        pwrBlink();

        // break
        if (_stop && isFiring) {
          _min = 1;
          sec = 0;
          isFiring = false;
          _stop = false;

          beep();
          
          lcd.setCursor(0, 1);
          for (uint8_t i = 0; i <= 13; i++) {
            lcd.print("\32\0");
          }
          lcd.setCursor(0, 1);
          lcd.print("Break:\0");

        } 
        // firing
        else if (_stop && !isFiring) {
          _min = 5;
          sec = 0;
          isFiring = true;
          _stop = false;

          beep();

          lcd.setCursor(0, 1);
          for (uint8_t i = 0; i <= 13; i++) {
            lcd.print("\32\0");
          }
          lcd.setCursor(0, 1);
          lcd.print("Firing:\0");

          _step++;
          if(_step < 11) {
            lcd.setCursor(6, 2);
            lcd.print(String(_step));
            lcd.print("/10\0");
          }
        }

        if (isFiring && _step < 11) {
          digitalWrite(relay, HIGH);
        }
        else {
          digitalWrite(relay, LOW);
        }
      }
      else {
        reset();
        finishScreen();
        printInstructions();
        checkMode();
      }
    }
  }
}

void clearStrings() {
  for (uint8_t i = 1; i <= 3; i++) {
    lcd.setCursor(0, i);
    for (uint8_t j = 0; j <= 19; j++) {
      lcd.print("\32\0");
    }
  }
}

void printInstructions() {
  lcd.setCursor(0, 1);
  lcd.print("--------------------\0");
  lcd.setCursor(0, 2);
  lcd.print("1. Select a mode\0");
  lcd.setCursor(0, 3);
  lcd.print("2. Press \"Start\"\0");
}

void timer() {

  if (!_stop) {

      uint32_t currentMillis = millis();

      if (currentMillis - previousMillis >= interval) {
        if (sec == 0 && _min > 0) {
          sec = 60;
          _min = _min - 1;
        }
        sec = sec - 1;

        previousMillis = currentMillis;   

        if (isFiring) {
          lcd.setCursor(8, 1);
        }
        else {
          lcd.setCursor(7, 1);
        }
        
        lcd.print("0\0");
        lcd.print(String(_min));
        if (sec >= 10) {
          lcd.print(":\0");
          lcd.print(String(sec));
        }
        else {
          lcd.print(":0\0");
          lcd.print(String(sec));
        }
      }
    }

  if (sec == 0 && _min == 0) {
    _stop = true;
  }
}

void totalTimer() {

  if (!finish) {

    uint32_t totalCurrentMillis = millis();

    if (totalCurrentMillis - totalPreviousMillis >= interval) {
      if (totalSec == 0 && totalMin > 0) {
        totalSec = 60;
        totalMin = totalMin - 1;
      }
      totalSec = totalSec - 1;

      totalPreviousMillis = totalCurrentMillis;   

      lcd.setCursor(11, 3);
      
      if (totalMin >= 10) {
        lcd.print(String(totalMin));
      }
      else {
        lcd.print("0\0");
        lcd.print(String(totalMin));
      }

      if (totalSec >= 10) {
        lcd.print(":\0");
        lcd.print(String(totalSec));
      }
      else {
        lcd.print(":0\0");
        lcd.print(String(totalSec));
      }
    }
  }

  if (totalSec == 0 && totalMin == 0) {
    finish = true;
  }
}

void reset() {
  
  digitalWrite(pwrLED, HIGH);
  digitalWrite(relay, LOW);
  
  sec = 1;
  _min = 0;
  previousMillis = 0;
  totalSec = 1;
  totalMin = 60;
  totalPreviousMillis = 0;
  _step = 1;

  start = false;
  _stop = false;
  isFiring = false;
  finish = false;

}

void pwrBlink() {

  uint32_t blinkCurrentMillis = millis();

  if (blinkCurrentMillis - blinkPreviousMillis >= interval) {
    
    blinkPreviousMillis = blinkCurrentMillis;
    pwrLEDState = !pwrLEDState;
    pwrLEDState ? digitalWrite(pwrLED, HIGH) : digitalWrite(pwrLED, LOW);
    
  }
}

void finishScreen() {
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("Firing is finished\0");
  
  for (uint8_t i = 1; i <= 3; i++) {
    for (uint8_t j = 0; j < 3; j++) {
      tone(buzzer, 1700);
      delay(200);
      noTone(buzzer);
      delay(200);
    }
    delay(1000);
  }

}

void checkMode() {
  if(digitalRead(modeBtn)) {
    lcd.setCursor(0, 0);
    lcd.print("Mode: main\0");
    mode = true;
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Mode: backup\0");
    mode = false;
  }
  beep();
}

void beep() {
  tone(buzzer, 1700, 70);
}