#include "Timer.h"

const int COUNTER_T_quick=5;//快闪间隔，灯由暗到亮的时间，5x时间终断间隔20ms=100ms，闪烁周期为200ms
const int COUNTER_T_slow=100;//慢闪间隔，灯由暗到亮的时间，100x时间终断间隔20ms=2000ms=2s，闪烁周期为4s

volatile int state = HIGH;//初始state设置为高电平
volatile int flag = LOW;//初始flag设置为低电平
int count = 0;//初始数量设置为0
volatile int COUNT_flash = 0;//初始闪烁周期设置为0

int buttonState = HIGH;//初始按钮状态设置为高电平
int lastButtonState = HIGH;//初始上一次按钮状态设置为高电平

void setup() {
//Serial.begin(9600);//初始串口波特率为9600，可以在对应串口监视器上看到输出
pinMode(GREEN_LED, OUTPUT);//绿灯引脚设置为输出模式
digitalWrite(GREEN_LED, state);//将绿灯电平状态设置为state
pinMode(PUSH2, INPUT_PULLUP);//将Push2引脚设置为上拉输入
COUNT_flash = COUNTER_T_slow;//让闪烁周期先等于慢闪
SetTimer(isrTimer, 20);//设置定时中断服务，每20ms执行一次isrTimer
}

void Push2_Dect()//Push2检测函数
{
  buttonState = digitalRead(PUSH2);//让按钮当前状态等于Push2当前状态

  if(buttonState != lastButtonState)//如果满足按钮状态不等于上一次按钮状态，即按钮状态发生改变，即发生一次按钮点击
  {
    count = 0;//使按钮从0开始计数
    state= HIGH;//state设置为高电平
    digitalWrite(GREEN_LED, state);//将绿灯引脚设置为高电平，即点亮绿灯
    
    if(buttonState == LOW)//按钮按下时，buttonState为低电平，此时令周期为快闪
    {
      COUNT_flash = COUNTER_T_quick;
    }
    else//按钮放开时，buttonState为高电平，此时令周期为慢闪
    {
      COUNT_flash = COUNTER_T_slow;
    }
  }
  lastButtonState = buttonState;//迭代buttonState
}

void isrTimer(void)
{
  Push2_Dect();

  if(++count >= COUNT_flash)//一旦count x 20ms  >= 快闪周期
  {
    flag = 1;//满足条件flag指示信号设置为1，即满足条件
    count = 0;//重新开始计时
  }
}

void loop() {
  if(flag){//一旦满足时长条件
    state = !state;//state反转
    digitalWrite(GREEN_LED, state);//绿灯反转
    flag = LOW;//flag不满足
  }
}
