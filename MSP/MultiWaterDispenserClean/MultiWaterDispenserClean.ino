#include "Timer.h"

// --- 引脚定义 (如果未在 Timer.h 中定义，请在此处或您的主 .ino 文件中定义) ---
// 例如:
// const int PUSH1 = D1; // 童锁按钮引脚
// const int PUSH2 = D2; // 热水按钮引脚
// const int RED_LED = D3; // 红色LED引脚
// const int GREEN_LED = D4; // 绿色LED引脚

const int push1Unlock = 300;
const int push2Overtime = 300;

// 机器状态初始为0状态，红灯初始状态为点亮状态，绿灯初始状态为熄灭状态
volatile int machineState = 0; // 0:锁定, 1:解锁, 2:出水中, 6:清洁循环
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

// 新增：用于清洁循环的状态和变量
const int CLEANING_CYCLE_STATE = 6;       // 定义清洁循环的状态值
const int CLEANING_CYCLE_DURATION = 3000; // 3000 * 10ms = 30秒，清洁循环周期

volatile bool p1AndP2WereSimultaneouslyPressedInState1 = false; // 标记在状态1中P1和P2是否曾被同时按下
volatile int cleaningCycleDurationTimer = 0;                    // 清洁循环持续时间计时器

void push1Dect() // push1检测函数
{
  if (machineState == CLEANING_CYCLE_STATE)
  {
    // 在清洁循环中，PUSH1无效。读取状态以保持lastPush1ButtonState为最新。
    lastPush1ButtonState = digitalRead(PUSH1);
    return;
  }

  if (machineState == 0) // 处于状态0时push1才会有效
  {
    push1ButtonState = digitalRead(PUSH1); // 将push1的电平值赋值给push1ButtonState

    if (push1ButtonState != lastPush1ButtonState) // 如果push1电平发生改变，理论上是发生一次按下
    {
      push1Timer = 0;                          // 此时push1计时器清零
      lastPush1ButtonState = push1ButtonState; // 迭代push1ButtonState
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

void overtimeDect() // push2按钮超时检测，未按下push2超时后会自动回到状态0
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

void push2Dect()
{
  if (machineState == CLEANING_CYCLE_STATE)
  {
    // 在清洁循环中，PUSH2无效。读取状态以保持lastPush2ButtonState为最新。
    lastPush2ButtonState = digitalRead(PUSH2);
    return;
  }

  push2ButtonState = digitalRead(PUSH2);

  if (push2ButtonState != lastPush2ButtonState)
  {
    if (push2ButtonState == LOW) // PUSH2 按下
    {
      push2Timer = 0; // 重置 push2Timer (当前未用于其他逻辑)
      if (machineState == 1)
      {
        stateTimer = 0; // 如果 PUSH2 被按下，则重置解锁状态超时
      }
    }
    else // PUSH2 松开
    {
      if (machineState == 1) // 如果在状态1 (解锁)
      {
        machineState = 2;  // 转换到状态2 (出水中)
        greenState = HIGH; // 绿灯亮 (出水)
        stateTimer = 0;    // 重置定时器
      }
      else if (machineState == 2) // 如果在状态2 (出水中)
      {
        machineState = 0; // 转换到状态0 (锁定)
        redState = HIGH;
        greenState = LOW;
      }
      push2Timer = 0;
    }
    lastPush2ButtonState = push2ButtonState;
  }
}

void isrTimer()
{
  int currentPush1 = digitalRead(PUSH1);
  int currentPush2 = digitalRead(PUSH2);

  // --- 清洁循环逻辑：进入与管理 ---
  if (machineState == 1)
  { // 仅在解锁状态下才能进入清洁循环
    if (currentPush1 == LOW && currentPush2 == LOW)
    {
      // 标记P1和P2已被同时按下
      p1AndP2WereSimultaneouslyPressedInState1 = true;
    }
    // 如果之前P1和P2被同时按下过，并且现在至少有一个按键松开了（即不再处于P1和P2都按下的状态）
    else if (p1AndP2WereSimultaneouslyPressedInState1 && !(currentPush1 == LOW && currentPush2 == LOW))
    {
      machineState = CLEANING_CYCLE_STATE;              // 进入清洁循环状态
      greenState = HIGH;                                // 绿灯亮
      redState = LOW;                                   // 红灯灭
      cleaningCycleDurationTimer = 0;                   // 重置清洁循环计时器
      p1AndP2WereSimultaneouslyPressedInState1 = false; // 重置标志位
    }
  }
  else if (machineState != CLEANING_CYCLE_STATE)
  {
    // 如果机器状态不是解锁状态（状态1），也不是清洁状态，则重置“同时按下”标志位
    p1AndP2WereSimultaneouslyPressedInState1 = false;
  }

  // --- 清洁循环周期管理与退出 ---
  if (machineState == CLEANING_CYCLE_STATE)
  {
    cleaningCycleDurationTimer++;
    if (cleaningCycleDurationTimer >= CLEANING_CYCLE_DURATION)
    {                                                   // CLEANING_CYCLE_DURATION = 3000 (30秒)
      machineState = 0;                                 // 清洁完成，返回到锁定状态 (状态0)
      greenState = LOW;                                 // 绿灯灭
      redState = HIGH;                                  // 红灯亮
      cleaningCycleDurationTimer = 0;                   // 重置计时器
      p1AndP2WereSimultaneouslyPressedInState1 = false; // 确保标志位被重置
    }
  }

  // --- 常规按键/状态逻辑 ---
  if (machineState != CLEANING_CYCLE_STATE)
  {
    // 如果不处于清洁循环状态，则执行常规的按键检测和超时逻辑
    push1Dect();
    push2Dect();
    overtimeDect();
  }
  else
  {
    // 如果处于清洁循环状态，为了确保在退出清洁循环后 lastButtonState 是最新的，
    // 我们在这里更新它们。因为 push1Dect 和 push2Dect 在清洁模式下会提前返回。
    lastPush1ButtonState = currentPush1;
    lastPush2ButtonState = currentPush2;
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

  SetTimer(isrTimer, 10); // 定时器中断周期为10ms
}

void loop()
{
  // 主循环通常为空，所有逻辑在定时器中断中处理
}