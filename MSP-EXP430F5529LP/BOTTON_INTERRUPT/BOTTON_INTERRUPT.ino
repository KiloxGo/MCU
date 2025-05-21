#include "Timer.h"

const int COUNTER_T_quick = 5;
const int COUNTER_T_slow = 100;
volatile int state = HIGH;
volatile int flag = LOW;
int count = 0;
volatile int COUNT_flash = 0;

int buttonState = HIGH;
int  lastButtonState = HIGH;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, state);
  pinMode(PUSH2, INPUT_PULLUP);
  COUNT_flash = COUNTER_T_slow;
  SetTimer(isrTimer, 20);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (flag) {
    state = !state;
    digitalWrite(GREEN_LED, state);
    flag = LOW;
  }
}

void isrTimer(void) {
  Push2_Dect();

  if (++count >= COUNT_flash)
  {
    flag = 1;
    count = 0;
  }
}

void Push2_Dect() {
  buttonState = digitalRead(PUSH2);
  if (buttonState != lastButtonState) {
    count = 0;
    state = HIGH;
    digitalWrite(GREEN_LED, state);
    if (buttonState == LOW) {
      COUNT_flash = COUNTER_T_quick;
    }
    else
    {
      COUNT_flash = COUNTER_T_slow;
    }
  }
  lastButtonState = buttonState;
}
