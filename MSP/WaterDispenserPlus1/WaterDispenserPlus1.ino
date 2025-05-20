#include "Timer.h"

const int push1Unlock = 250;    
const int push2Overtime = 250;
const int maxTimeLengthOfTwiceClick = 15;

//机器状态初始为0状态，红灯初始状态为点亮状态，绿灯初始状态为熄灭状态
volatile int machineState = 0;  
volatile int redState = HIGH;   
volatile int greenState = LOW;  


//push1初始按钮状态为高电平，push1初始上一次按钮状态为高电平
//push2初始按钮状态为高电平，push2初始上一次按钮状态为高电平
int push1ButtonState = HIGH;
int lastPush1ButtonState = HIGH;
int push2ButtonState = HIGH;
int lastPush2ButtonState = HIGH;
int lastPush2ButtonState_for_clickDect = HIGH;

//push1定时器初始值为0，push2定时器初始值为0，状态定时器初始值为0
volatile int push1Timer = 0;     
volatile int push2Timer = 0;     
volatile int stateTimer = 0;
volatile int clickTimer = 0;     

volatile int clickCount = 0; //点击次数

void push1Dect()//push1检测函数
{
  if(machineState == 0)//处于状态0时push1才会有效
  {
    push1ButtonState = digitalRead(PUSH1);//将push1的电平值赋值给push1ButtonState

    if(push1ButtonState != lastPush1ButtonState)//如果push1电平发生改变，理论上是发生一次按下
    {
      push1Timer = 0;//此时push1计时器清零
      lastPush1ButtonState = push1ButtonState;//迭代push1ButtonState
    }
    
    if(push1ButtonState == LOW)//如果push1当前状态处于按下状态
    {
      push1Timer++;//开始计数
      if(push1Timer >= push1Unlock)//计数时间大于等于解锁时间
      {
        machineState = 1;//机器状态切换到1
        redState = LOW;//红灯灭
        stateTimer = 0;//状态1下的计时器清零
        push1Timer = 0;  
      }
    }
  }
}

void overtimeDect()//push2按钮超时检测，未按下push2超时后会自动回到状态0
{
  if(machineState == 1)//仅在状态1时生效
  {
    stateTimer++;//开始计数
    if(stateTimer >= push2Overtime)//大于超时时间
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
      // else if(machineState == 2)
      // {
      //   machineState = 0;
      //   redState = HIGH;
      //   greenState = LOW;
      // }
      push2Timer = 0;
    }
    lastPush2ButtonState = push2ButtonState;
  }
  
  if(push2ButtonState == LOW && machineState == 1)
  {
    stateTimer = 0;
  }
}

void clickDect() {
    // 仅在机器状态为2（放水状态）时执行双击检测逻辑
    if (machineState != 2) {
        clickCount = 0; // 如果不是放水状态，重置点击计数
        clickTimer = 0; // 重置点击计时器
        // 当不处于状态2时，确保 lastPush2ButtonState_for_clickDect 被初始化或更新
        // 读取当前PUSH2状态，因为引脚上拉，未按下时为HIGH
        lastPush2ButtonState_for_clickDect = digitalRead(PUSH2);
        return;
    }

    // 读取 PUSH2 当前状态
    // 注意: push2ButtonState 已在 isrTimer 开始时通过 push2Dect 读取并存储在全局变量中。
    // 我们直接使用全局的 push2ButtonState。
    int currentPush2State = push2ButtonState;

    // 如果 clickCount 为 1, 表示已经检测到第一次释放，正在等待第二次释放
    if (clickCount == 1) {
        clickTimer++; // 计时器递增 (每个 ISR 周期增加一次)
        // 检查是否超时 (超过了定义的最大双击间隔时间)
        if (clickTimer > maxTimeLengthOfTwiceClick) {
            clickCount = 0; // 超时，双击失败，重置点击计数
            clickTimer = 0; // 重置计时器
            // 此时机器应继续放水 (保持在 machineState = 2)
        }
    }

    // 检测 PUSH2 按钮状态是否发生变化 (边沿检测)
    // 使用 clickDect 专属的 lastPush2ButtonState_for_clickDect 进行比较
    if (currentPush2State != lastPush2ButtonState_for_clickDect) {
        if (currentPush2State == HIGH) { // 按钮被释放 (从按下状态变为高电平)
            if (clickCount == 0) {
                // 这是双击序列中的第一次释放
                clickCount = 1;   // 标记已发生第一次释放
                clickTimer = 0;   // 重置/启动双击间隔计时器
            } else { // clickCount == 1, 这意味着这是第二次释放
                     // 并且因为上面的超时检查，我们知道它在有效时间内
                // 双击成功!
                machineState = 0;   // 机器回到锁定状态 (状态0)
                redState = HIGH;    // 红灯亮
                greenState = LOW;   // 绿灯灭
                clickCount = 0;     // 重置点击计数
                clickTimer = 0;     // 重置计时器
            }
        }
        // 更新 clickDect 专属的 PUSH2 最后状态变量
        lastPush2ButtonState_for_clickDect = currentPush2State;
    }
}

// void clickDect()
// {
//   if(machineState == 2)
//   {
//     if(push2DectButtonState != lastPush2DectButtonState)
//   {
//     if(push2DectButtonState == HIGH && clickCount == 0)
//     {
//       clickTimer = 0;
//       clickTimer++;
//       clickCount++;
//     }
//     if(push2DectButtonState == HIGH && clickCount == 1)
//     {
//       if(clickTimer <= maxTimeLengthOfTwiceClick)
//       {
//         clickCount=0;
//         machineState = 0;
//         redState = HIGH;
//         greenState = LOW;
//       }
//       else
//       {
//         clickCount = 0;
//       }
//     }
//   }
//   }
//   lastPush2DectButtonState = push2DectButtonState;
// }

void isrTimer()
{
  push1Dect();
  push2Dect();
  overtimeDect();
  clickDect();
  
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
