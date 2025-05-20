#include "Timer.h"

int flag =0;
int count = 0;
int COUNT_flash = 100;

//设置push1童锁解锁时间为300 x 10ms = 3s
//设置push2超时为300 x 10ms = 3s
const int push1Unlock = 300;    
const int push2Overtime = 300;  

//机器状态初始为0状态，红灯初始状态为点亮状态，绿灯初始状态为熄灭状态
volatile int machineState = 0;  
volatile int redState = HIGH;   
volatile int greenState = LOW;  
volatile int state = HIGH;

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

void push1Dect() // PUSH1 检测函数
{
    if (machineState == 0) // PUSH1 在状态0 (锁定) 时有效
    {
        push1ButtonState = digitalRead(PUSH1);
        if (push1ButtonState != lastPush1ButtonState)
        {
            push1Timer = 0; // 按键状态改变时重置定时器
            lastPush1ButtonState = push1ButtonState;
        }
        if (push1ButtonState == LOW) // 如果 PUSH1 被按下
        {
            push1Timer++;
            if (push1Timer >= push1Unlock) // 如果按住时间足够长
            {
                machineState = 1; // 转换到状态1 (解锁)
                redState = LOW;   // 红灯灭
                stateTimer = 0;   // 重置状态1超时定时器
                push1Timer = 0;
            }
        }
    }
    else if (machineState == 2) // PUSH1 在状态2 (出水中) 时用于暂停功能
    {
        push1ButtonState = digitalRead(PUSH1);
        if (push1ButtonState != lastPush1ButtonState)
        {
            if (push1ButtonState == LOW) // 如果 PUSH1 被按下 (短按)
            {
                machineState = 3;       // 转换到状态3 (暂停)
                redState = LOW;         // 红灯灭
                greenState = LOW;       // 逻辑上停止出水
                
                // === 设置状态3闪烁绿灯 ===
                ::state = HIGH;         // 开始时闪烁LED为亮 (使用全局 'state')
                digitalWrite(GREEN_LED, ::state); // 立即更新物理LED
                ::count = 0;            // 重置闪烁计数器
                ::flag = LOW;           // 清除标志位，等待isrTimer为下次切换设置它
                // === 设置结束 ===
            }
        }
        lastPush1ButtonState = push1ButtonState;
    }
    else if (machineState == 3) // PUSH1 在状态3 (暂停) 时用于返回锁定状态
    {
        push1ButtonState = digitalRead(PUSH1);
        if (push1ButtonState != lastPush1ButtonState)
        {
            if (push1ButtonState == LOW) // 如果 PUSH1 被按下
            {
                machineState = 0;   // 转换到状态0 (锁定)
                redState = HIGH;    // 红灯亮
                greenState = LOW;   // 绿灯灭 (停止出水)
                // 当 machineState 不再是3时，isrTimer会用 'greenState' 更新 GREEN_LED
            }
        }
        lastPush1ButtonState = push1ButtonState;
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

void push2Dect() // PUSH2 检测函数
{
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
                machineState = 2;   // 转换到状态2 (出水中)
                greenState = HIGH;  // 绿灯亮 (出水)
                stateTimer = 0;     // 重置定时器
            }
            else if (machineState == 2) // 如果在状态2 (出水中)
            {
                machineState = 0;   // 转换到状态0 (锁定)
                redState = HIGH;    // 红灯亮
                greenState = LOW;   // 绿灯灭 (停止出水)
            }
            else if (machineState == 3) // 如果在状态3 (暂停)
            {
                machineState = 2;   // 转换到状态2 (出水中)
                greenState = HIGH;  // 绿灯亮 (出水)
                redState = LOW;     // 确保红灯为灭
                // 当 machineState 不再是3时，isrTimer会用 'greenState' 更新 GREEN_LED
            }
            push2Timer = 0; // 重置 push2Timer
        }
        lastPush2ButtonState = push2ButtonState;
    }
}

void isrTimer()
{
    push1Dect();
    push2Dect();
    overtimeDect();

    // 状态3的闪烁逻辑
    if (machineState == 3) {
        if (++::count >= COUNT_flash) { // COUNT_flash = 100，用于1秒间隔 (100 * 10ms)
            ::flag = 1; // 通知 loop() 切换闪烁LED的状态
            ::count = 0;
        }
    } else {
        // 如果不在状态3，则重置闪烁辅助变量。
        // 此处重置 'flag' 和 'count' 以确保它们不会错误地持续存在。
        // '::state' (闪烁器的亮/灭状态) 不在此处重置；
        // LED状态由 'greenState' 管理或在转换时设置。
        ::flag = 0;
        ::count = 0;
    }

    // 更新物理LED
    digitalWrite(RED_LED, redState);

    // 绿灯控制：
    // 如果在状态3，绿灯闪烁并由 loop() 中的 '::state' 控制。
    // 闪烁绿灯的初始状态在转换到状态3时设置。
    // 如果不在状态3，绿灯由 'greenState' 变量控制 (常亮/灭)。
    if (machineState != 3) {
        digitalWrite(GREEN_LED, greenState);
    }
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
   if (::flag && machineState == 3) {
        ::state = !::state; // 切换闪烁LED的亮/灭状态
        digitalWrite(GREEN_LED, ::state); // 更新物理绿灯
        ::flag = LOW;       // 重置标志位
    }
}
