#include "Timer.h"

const int SIMULTANEOUS_PRESS_DETECT_TIME = 100; //  1秒，用于检测按键同时按下的时间
const int ERROR_STATE_TIMEOUT = 500;           //  5秒，错误状态的超时时间
const int ERROR_FLASH_INTERVAL = 20;           //  200毫秒，错误状态下LED快速闪烁的半周期

volatile int simultaneouslyPressedTimer = 0; // PUSH1和PUSH2同时按下计时器
volatile int errorStateDurationTimer = 0;    // 机器处于状态4的持续时间计时器
volatile int errorFlashCounter = 0;          // 错误状态下LED闪烁计时器
volatile int errorFlashToggleState = LOW;    // 错误状态下LED闪烁的当前亮/灭状态

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
    // --- 错误检测逻辑 (可以从任何非错误状态触发) ---
    int currentP1State = digitalRead(PUSH1); //读取PUSH1当前状态
    int currentP2State = digitalRead(PUSH2); //读取PUSH2当前状态

    if (currentP1State == LOW && currentP2State == LOW) { //如果PUSH1和PUSH2都按下
        if (machineState != 4) { // 仅当不处于错误状态时才计时
            simultaneouslyPressedTimer++;
            if (simultaneouslyPressedTimer >= SIMULTANEOUS_PRESS_DETECT_TIME) {
                machineState = 4;               // 进入错误状态 (状态4)
                // 初始化错误状态相关变量
                errorStateDurationTimer = 0;
                errorFlashCounter = 0;
                errorFlashToggleState = HIGH;   // 开始闪烁时，LED先亮
                // redState 和 greenState 将在下面的状态4逻辑中设置
                simultaneouslyPressedTimer = 0; // 触发后重置计时器
            }
        } else {
            // 如果已处于状态4且按键仍被按下, 保持 simultaneouslyPressedTimer 为0或不作处理,
            // 因为当前错误状态退出逻辑是基于超时的。此处重置无妨。
            simultaneouslyPressedTimer = 0;
        }
    } else {
        // 如果按键未同时按下，则重置计时器
        simultaneouslyPressedTimer = 0;
    }

    // --- 状态机逻辑 ---
    if (machineState == 4) {
        // 当前处于错误状态 (状态4)
        errorStateDurationTimer++;
        if (errorStateDurationTimer >= ERROR_STATE_TIMEOUT) {
            // 错误状态超时：返回到状态0 (锁定)
            machineState = 0;
            redState = HIGH;    // 锁定状态：红灯亮
            greenState = LOW;   // 绿灯灭
        } else {
            // 仍在错误状态：处理LED快速闪烁
            errorFlashCounter++;
            if (errorFlashCounter >= ERROR_FLASH_INTERVAL) {
                errorFlashToggleState = !errorFlashToggleState; // 切换闪烁状态
                errorFlashCounter = 0; // 重置闪烁间隔计数器
            }
            // 设置LED以进行闪烁
            redState = errorFlashToggleState;
            greenState = errorFlashToggleState;
        }
    } else {
        // 正常操作 (状态 0, 1, 2)
        // PUSH1/PUSH2的检测函数会自行读取引脚状态
        push1Dect();
        push2Dect();
        overtimeDect();
        // 这些函数会根据其内部逻辑更新 redState 和 greenState
    }

    // 根据当前状态逻辑设置的 redState 和 greenState 来更新物理LED
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
