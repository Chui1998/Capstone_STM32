#include "key.h"
#include "delay.h" 

//按键初始化函数
void KEY_Init(void)
{
	RCC->AHB1ENR|=1<<0;     //使能PORTA时钟 
	RCC->AHB1ENR|=1<<2;     //使能PORTC时钟 
	RCC->AHB1ENR|=1<<7;     //使能PORTH时钟
	GPIO_Set(GPIOA,PIN0,GPIO_MODE_IN,0,0,GPIO_PUPD_PD); 			//PA0设置为下拉输入 
	GPIO_Set(GPIOC,PIN13,GPIO_MODE_IN,0,0,GPIO_PUPD_PU); 			//PC13设置为上拉输入 
	GPIO_Set(GPIOH,PIN2|PIN3,GPIO_MODE_IN,0,0,GPIO_PUPD_PU);		//PH2/3设置上拉输入
} 
//按键处理函数
//返回按键值
//mode:0,不支持连续按;1,支持连续按;
//0，没有任何按键按下
//1，KEY0按下
//2，KEY1按下
//3，KEY2按下 
//4，KEY_UP按下 即WK_UP
//注意此函数有响应优先级,KEY0>KEY1>KEY2>KEY_UP!!
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//按键按松开标志
	if(mode)key_up=1;  //支持连按		  
	if(key_up&&(KEY0==0||KEY1==0||KEY2==0||WK_UP==1))
	{
		delay_ms(10);//去抖动 
		key_up=0;
		if(KEY0==0)return 1;
		else if(KEY1==0)return 2;
		else if(KEY2==0)return 3;
		else if(WK_UP==1)return 4;
	}else if(KEY0==1&&KEY1==1&&KEY2==1&&WK_UP==0)key_up=1; 	    
 	return 0;// 无按键按下
}
