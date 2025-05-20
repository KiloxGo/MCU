#include "Timer.h"

// --- 常规设置 ---
const int push1Unlock = 300;   // PUSH1解锁时间 (300 * 10ms = 3s)
const int push2Overtime = 300; // PUSH2在解锁状态下的超时时间 (300 * 10ms = 3s)
const int preEnterTime = 500;  // 进入预冲洗模式的长按PUSH2时间 (500 * 10ms = 5s)

// --- 机器状态和LED状态 ---
volatile int machineState = 0; // 0:锁定, 1:解锁, 2:出水中, 5:预冲洗模式
volatile int redState = HIGH;  // 红色LED状态 (HIGH为亮)
volatile int greenState = LOW; // 绿色LED状态 (HIGH为亮)

// --- 按键状态变量 ---
int push1ButtonState = HIGH;
int lastPush1ButtonState = HIGH;
int push2ButtonState = HIGH;
int lastPush2ButtonState = HIGH;

// --- 定时器变量 ---
volatile int push1Timer = 0; // PUSH1长按计时器
volatile int push2Timer = 0; // PUSH2相关计时器 (在当前逻辑中主要用于重置)
volatile int stateTimer = 0; // 状态1 (解锁) 的超时计时器
volatile int preTimer = 0;   // 用于检测从状态0长按PUSH2进入预冲洗模式的计时器

// --- 预冲洗模式新增变量 ---
const int PRE_RINSE_STATE = 5;            // 定义预冲洗模式的状态值
volatile int preRinseDurationTimer = 0;   // 预冲洗模式持续时间计时器
const int PRE_RINSE_CYCLE_DURATION = 500; // 500 * 10ms = 5秒，预冲洗周期

void push1Dect() // PUSH1检测函数
{
  if (machineState == PRE_RINSE_STATE)
  {
    // 在预冲洗模式下，PUSH1不起作用
    // 读取按键状态以保持lastPush1ButtonState的更新
    lastPush1ButtonState = digitalRead(PUSH1);
    return;
  }

  if (machineState == 0) // 处于状态0时push1才会有效
  {
    push1ButtonState = digitalRead(PUSH1); // 将push1的电平值赋值给push1ButtonState

    if (push1ButtonState != lastPush1ButtonState) // 如果push1电平发生改变
    {
      push1Timer = 0;                          // 此时push1计时器清零
      lastPush1ButtonState = push1ButtonState; // 迭代push1ButtonState
    }

    if (push1ButtonState == LOW) // 如果push1当前状态处于按下状态
    {
      push1Timer++;                  // 开始计数
      if (push1Timer >= push1Unlock) // 计数时间大于等于解锁时间
      {
        machineState = 1; // 机器状态切换到1 (解锁)
        redState = LOW;   // 红灯灭
        stateTimer = 0;   // 状态1下的计时器清零
        push1Timer = 0;
      }
    }
  }
  // 如果 machineState 不是 0 或 PRE_RINSE_STATE (例如 1 或 2),
  // 根据原始代码结构 PUSH1 当前是无效的 (仅包含 machineState == 0 的逻辑)。
}

void overtimeDect() // PUSH2按钮超时检测，未按下PUSH2超时后会自动回到状态0 (仅在状态1生效)
{
  if (machineState == 1) // 仅在状态1时生效
  {
    stateTimer++;                    // 开始计数
    if (stateTimer >= push2Overtime) // 大于超时时间
    {
      machineState = 0; // 返回锁定状态
      redState = HIGH;
      stateTimer = 0;
    }
  }
}

void push2Dect() // PUSH2检测函数
{
  push2ButtonState = digitalRead(PUSH2);

  if (machineState == PRE_RINSE_STATE)
  {
    // 在预冲洗模式期间，PUSH2的按键不改变状态。
    // 更新lastPush2ButtonState以避免在退出预冲洗模式时产生错误的边缘检测。
    lastPush2ButtonState = push2ButtonState;
    return;
  }

  // --- 从状态0长按PUSH2进入预冲洗模式 ---
  if (machineState == 0)
  {
    if (push2ButtonState == LOW)
    { // 如果PUSH2当前被按下
      preTimer++;
      if (preTimer >= preEnterTime)
      { // preEnterTime = 500 (5秒)
        machineState = PRE_RINSE_STATE;
        greenState = HIGH;         // 绿灯亮
        redState = LOW;            // 红灯（童锁指示灯）灭
        preRinseDurationTimer = 0; // 启动预冲洗周期计时器
        preTimer = 0;              // 重置长按计时器
      }
    }
    else
    {               // PUSH2 在状态0时未被按下 (为HIGH)
      preTimer = 0; // 重置长按计时器
    }
    // 对于状态0，PUSH2的边缘变化（按下/松开）不直接导致其他状态转换，只有长按有效。
    // 因此，在此处更新 lastPush2ButtonState 很重要。
    lastPush2ButtonState = push2ButtonState;
    return; // 完成对状态0下PUSH2的处理
  }

  // --- 状态1和状态2下PUSH2的常规边缘检测逻辑 ---
  if (push2ButtonState != lastPush2ButtonState)
  {
    if (push2ButtonState == LOW) // PUSH2 按下 (边缘)
    {
      push2Timer = 0; // 根据您的原始代码
      if (machineState == 1)
      {
        stateTimer = 0; // 如果 PUSH2 被按下，则重置解锁状态超时
      }
    }
    else // PUSH2 松开 (边缘)
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
      push2Timer = 0; // 根据您的原始代码
    }
    lastPush2ButtonState = push2ButtonState; // 更新上次按键状态
  }
}

void isrTimer()
{
  // --- 预冲洗模式周期管理 ---
  if (machineState == PRE_RINSE_STATE)
  {
    preRinseDurationTimer++;
    if (preRinseDurationTimer >= PRE_RINSE_CYCLE_DURATION)
    {                            // PRE_RINSE_CYCLE_DURATION = 500 (5秒)
      machineState = 0;          // 返回到锁定状态 (状态0)
      greenState = LOW;          // 绿灯灭
      redState = HIGH;           // 红灯亮
      preRinseDurationTimer = 0; // 重置计时器
                                 // PUSH1 和 PUSH2 将根据状态0的逻辑重新变为活动状态
    }
  }

  // 调用检测函数。它们可能会改变 machineState。
  push1Dect();
  push2Dect();
  overtimeDect(); // 仅当 machineState 为 1 时起作用

  digitalWrite(RED_LED, redState);
  digitalWrite(GREEN_LED, greenState);
}

void setup()
{
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, redState);     // 初始化红色LED
  digitalWrite(GREEN_LED, greenState); // 初始化绿色LED

  pinMode(PUSH1, INPUT_PULLUP); // PUSH1上拉输入
  pinMode(PUSH2, INPUT_PULLUP); // PUSH2上拉输入

  SetTimer(isrTimer, 10); // 设置定时器中断，每10ms一次
}

void loop()
{
  // 主循环通常保持为空，因为所有逻辑都在定时器中断服务程序中处理
}