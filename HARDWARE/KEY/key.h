#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h" 

#define KEY0 		GPIO_Pin_Get(GPIOH,PIN3)   	//PH3
#define KEY1 		GPIO_Pin_Get(GPIOH,PIN2)		//PH2 
#define KEY2 		GPIO_Pin_Get(GPIOC,PIN13)	//PC13
#define WK_UP 		GPIO_Pin_Get(GPIOA,PIN0)	//PA0 

#define motor_side(x)		GPIO_Pin_Set(GPIOB,PIN12,x)

#define KEY0_PRES 	1	//KEY0按下
#define KEY1_PRES	2	//KEY1按下
#define KEY2_PRES	3	//KEY2按下
#define WKUP_PRES   4	//KEY_UP按下(即WK_UP)

void KEY_Init(void);		//IO初始化
u8 KEY_Scan(u8);  		//按键扫描函数					    
#endif
