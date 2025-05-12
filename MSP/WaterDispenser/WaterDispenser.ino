#include "Timer.h"

const int push1Unlock = 150;    
const int push2Overtime = 150;  

volatile int machineState = 0;  
volatile int redState = HIGH;   
volatile int greenState = LOW;  

int push1ButtonState = HIGH;
int lastPush1ButtonState = HIGH;
int push2ButtonState = HIGH;
int lastPush2ButtonState = HIGH;

volatile int push1Timer = 0;     
volatile int push2Timer = 0;     
volatile int stateTimer = 0;     

void push1Dect()
{
  if(machineState == 0)
  {
    push1ButtonState = digitalRead(PUSH1);

    if(push1ButtonState != lastPush1ButtonState)
    {
      push1Timer = 0;
      lastPush1ButtonState = push1ButtonState;
    }
    
    if(push1ButtonState == LOW)
    {
      push1Timer++;
      if(push1Timer >= push1Unlock)
      {
        machineState = 1;
        redState = LOW;
        stateTimer = 0;  
        push1Timer = 0;  
      }
    }
  }
}

void overtimeDect()
{
  if(machineState == 1)
  {
    stateTimer++;
    if(stateTimer >= push2Overtime)
    {
      machineState = 0;
      redState = HIGH;
      stateTimer = 0;
    }
  }
}

void push2Dect()
{
  push2ButtonState = digitalRead(PUSH2);
  
  if(push2ButtonState != lastPush2ButtonState)
  {
    if(push2ButtonState == LOW)
    {
      push2Timer = 0;
    }
    else
    {
      if(machineState == 1)
      {
        machineState = 2;
        greenState = HIGH;
        stateTimer = 0;
      }
      else if(machineState == 2)
      {
        machineState = 0;
        redState = HIGH;
        greenState = LOW;
      }
      push2Timer = 0;
    }
    lastPush2ButtonState = push2ButtonState;
  }
  
  if(push2ButtonState == LOW && machineState == 1)
  {
    stateTimer = 0;
  }
}

void isrTimer()
{
  push1Dect();
  push2Dect();
  overtimeDect();
  
  digitalWrite(RED_LED, redState);
  digitalWrite(GREEN_LED, greenState);
}

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  
  digitalWrite(RED_LED, redState);
  digitalWrite(GREEN_LED, greenState);
  
  pinMode(PUSH1, INPUT_PULLUP);
  pinMode(PUSH2, INPUT_PULLUP);
  
  SetTimer(isrTimer, 20);
}

void loop() {
}
