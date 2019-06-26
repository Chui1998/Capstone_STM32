#ifndef __LCD1602_H
#define __LCD1602_H
#include "sys.h"
/**************************************************************
开发板              LCD1602
PE7~PE14 ->     D0~D7
PD14          ->     RS
PD15          ->     RW
PD0            ->     E
**************************************************************/
#define LCD1602_RS_OUT(x) 		GPIO_Pin_Set(GPIOD,PIN14,x) 		//LCD1602_RS引脚输出电平宏定义
#define LCD1602_RW_OUT(x) 		GPIO_Pin_Set(GPIOD,PIN15,x)	 	//LCD1602_RW引脚输出电平宏定义
#define LCD1602_E_OUT(x) 			GPIO_Pin_Set(GPIOD,PIN0,x)			//LCD1602_E引脚输出电平宏定义
#define LCD1602_DATA_OUT(x)     GPIOE->ODR=x<<7                         //LCD1602

void LCD1602_Init(void);
void LCD1602_Write_Command(u8 dat);
void LCD1602_Write_Data(u8 dat);
void LCD1602_Display_String(u8 x,u8 y,const u8 *p,u8 length);
void LCD1602_Display_Char(u8 x,u8 y,u8 cha);
void LCD1602_Display_Num(u8 x,u8 y,u16 num);
void LCD1602_Clear_Line(u8 y);
void LCD1602_Display_Menu(u8 menu);
	
#endif
