#include "Timer.h"

const int push1Unlock = 300;    
const int push2Overtime = 300;  

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

//push1定时器初始值为0，push2定时器初始值为0，状态定时器初始值为0
volatile int push1Timer = 0;     
volatile int push2Timer = 0;     
volatile int stateTimer = 0;     

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
                machineState = 2;   // 转换到状态2 (出水中)
                greenState = HIGH;  // 绿灯亮 (出水)
                stateTimer = 0;     // 重置定时器
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
  
  SetTimer(isrTimer, 10);
}

void loop() {
}
