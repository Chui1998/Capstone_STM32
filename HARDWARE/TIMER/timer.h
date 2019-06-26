#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
	
//********************************************************************************
//修改说明
//新增TIM3_PWM_Init函数,用于PWM输出	
//新增TIM5_CH1_Cap_Init函数,用于输入捕获		  
////////////////////////////////////////////////////////////////////////////////// 	 

//通过改变TIM3->CCR4的值来改变占空比，从而控制LED0的亮度
#define PWM_VAL TIM3->CCR4    

extern u8 TIM5CH1_CAPTURE_STA;
extern u32 TIM5CH1_CAPTURE_VAL;

extern TIM_HandleTypeDef TIM3_Handler;      //定时器3PWM句柄 
extern TIM_OC_InitTypeDef TIM3_CH4Handler;  //定时器3通道4句柄
extern TIM_HandleTypeDef TIM4_Handler;
extern TIM_HandleTypeDef TIM5_Handler;      //定时器5句柄

void TIM3_PWM_Init(u16 arr,u16 psc); 		//定时器3 PWM初始化
void TIM4_Init(u16 arr,u16 psc);
void TIM5_CH1_Cap_Init(u32 arr,u16 psc); 	//初始化TIM5_CH1输入捕获

#endif























