// 谭畅平  524031910141  徐青菁

#include "Timer.h"
// 设置push2超时为150 x 20ms = 3s
const int push1Unlock = 300;
const int push2Overtime = 150;

// 机器状态初始为0状态，红灯初始状态为点亮状态，绿灯初始状态为熄灭状态
volatile int machineState = 0;
volatile int redState = HIGH;
volatile int greenState = LOW;

// push1初始按钮状态为高电平，push1初始上一次按钮状态为高电平
// push2初始按钮状态为高电平，push2初始上一次按钮状态为高电平
int push1ButtonState = HIGH;
int lastPush1ButtonState = HIGH;
int push2ButtonState = HIGH;
int lastPush2ButtonState = HIGH;

// push1定时器初始值为0，push2定时器初始值为0，状态定时器初始值为0
volatile int push1Timer = 0;
volatile int push2Timer = 0;
volatile int stateTimer = 0;
volatile bool bothButtonsPressed = false; // 记录两个按钮是否同时被按下过

void push1Detect() // push1检测函数
{
  if (machineState == 0 && !bothButtonsPressed) // 处于状态0且没有同时按下按钮的历史时push1才会有效
  {
    push1ButtonState = digitalRead(PUSH1); // 将push1的电平值赋值给push1ButtonState

    if (push1ButtonState != lastPush1ButtonState) // 如果push1电平发生改变，理论上是发生一次按下
    {
      lastPush1ButtonState = push1ButtonState; // 迭代push1ButtonState

      if (push1ButtonState == HIGH) // 释放按钮时重置计时器
      {
        push1Timer = 0;
      }
    }

    if (push1ButtonState == LOW) // 如果push1当前状态处于按下状态
    {
      push1Timer++;                  // 开始计数
      if (push1Timer >= push1Unlock) // 计数时间大于等于解锁时间
      {
        machineState = 1; // 机器状态切换到1
        redState = LOW;   // 红灯灭
        stateTimer = 0;   // 状态1下的计时器清零
        push1Timer = 0;
      }
    }
  }
}

void overtimeDetect() // push2按钮超时检测，未按下push2超时后会自动回到状态0
{
  if (machineState == 1) // 仅在状态1时生效
  {
    stateTimer++;                    // 开始计数
    if (stateTimer >= push2Overtime) // 大于超时时间
    {
      machineState = 0;
      redState = HIGH;
      stateTimer = 0;
    }
  }
}

void push2Detect()
{
  push2ButtonState = digitalRead(PUSH2);

  if (push2ButtonState != lastPush2ButtonState)
  {
    if (push2ButtonState == LOW)
    {
      push2Timer = 0;
    }
    else // 按钮释放
    {
      if (machineState == 1)
      {
        machineState = 2;
        greenState = HIGH;
        stateTimer = 0;
      }
      else if (machineState == 2)
      {
        machineState = 0;
        redState = HIGH;
        greenState = LOW;
      }
      push2Timer = 0;
    }
    lastPush2ButtonState = push2ButtonState;
  }

  if (push2ButtonState == LOW && machineState == 1)
  {
    stateTimer = 0;
  }
}

void isrTimer()
{
  int currentP1State = digitalRead(PUSH1); // 读取PUSH1当前状态
  int currentP2State = digitalRead(PUSH2); // 读取PUSH2当前状态

  // 处理两个按键同时按下的情况
  if (currentP1State == LOW && currentP2State == LOW)
  {
    bothButtonsPressed = true; // 标记两个按钮曾经同时按下

    if (machineState != 4) // 仅当不处于错误状态时才计时
    {
      push1Timer++;
      if (push1Timer >= push1Unlock)
      {
        machineState = 4; // 进入错误状态 (状态4)
        redState = HIGH;  // 锁定状态：红灯亮
        greenState = LOW; // 绿灯灭
        push1Timer = 0;   // 触发后重置计时器
      }
    }
  }
  else if (currentP1State == HIGH && currentP2State == HIGH)
  {
    // 两个按钮都释放时，如果在错误状态，恢复到状态0
    if (machineState == 4)
    {
      machineState = 0;
      redState = HIGH;
      greenState = LOW;
    }

    // 按钮都释放后，清除同时按下的标记
    bothButtonsPressed = false;
    push1Timer = 0;
  }

  // 在没有同时按下按键时，才执行正常的按键检测
  if (!(currentP1State == LOW && currentP2State == LOW))
  {
    push1Detect();
    push2Detect();
    overtimeDetect();
  }

  digitalWrite(RED_LED, redState);
  digitalWrite(GREEN_LED, greenState);
}

void setup()
{
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, redState);
  digitalWrite(GREEN_LED, greenState);

  pinMode(PUSH1, INPUT_PULLUP);
  pinMode(PUSH2, INPUT_PULLUP);

  SetTimer(isrTimer, 20);
}

void loop()
{
}
