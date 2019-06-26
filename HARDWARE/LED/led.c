#include "led.h"
#include "sys.h"

//初始化PB0,PB1为输出.并使能这两个口的时钟		    
//LED IO初始化
void LED_Init(void)
{
	RCC->AHB1ENR|=1<<1;	//使能PORTB时钟 
	GPIO_Set(GPIOB,PIN0|PIN1,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU); //PB0,PB1设置
	LED0(1);			//关闭DS0
	LED1(0);			//开启DS1
}
