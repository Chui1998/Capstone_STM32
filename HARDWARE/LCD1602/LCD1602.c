#include "LCD1602.h"
#include "sys.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"

/**************************************************************
开发板              LCD1602
PE7~PE14 ->     D0~D7
PD14          ->     RS
PD15          ->     RW
PD0            ->     E
**************************************************************/
/*****************************************************************************
函数：LCD1602写命令函数
输入：写入的命令
输出：无
作用：向LCD1602写入命令。
******************************************************************************/
void LCD1602_Write_Command(u8 dat)
{
	taskENTER_CRITICAL();           //进入临界区
	LCD1602_E_OUT(0);
	LCD1602_RS_OUT(0);
	LCD1602_RW_OUT(0);
	
	LCD1602_DATA_OUT(dat);   //GPIOE->ODR=dat<<7;
	delay_ms(1);
	
	LCD1602_E_OUT(1);
	delay_ms(5);
	LCD1602_E_OUT(0);
	taskEXIT_CRITICAL();            //退出临界区
}

/*****************************************************************************
函数：LCD1602写数据函数
输入：写入的数据
输出：无
作用：向LCD1602写入数据。
******************************************************************************/
void LCD1602_Write_Data(u8 dat)
{
	taskENTER_CRITICAL();           //进入临界区
	LCD1602_E_OUT(0);
	LCD1602_RS_OUT(1);
	LCD1602_RW_OUT(0);
	
    LCD1602_DATA_OUT(dat);
	delay_ms(1);
	
	LCD1602_E_OUT(1);
	delay_ms(5);
	LCD1602_E_OUT(0);
	taskEXIT_CRITICAL();            //退出临界区
}

/*****************************************************************************
函数：LCD1602初始化函数
输入：无
输出：无
作用：初始化引脚和LCD1602。
******************************************************************************/
void LCD1602_Init(void)
{
	RCC->AHB1ENR|=3<<3;                                 //使能GPIOD和GPIOE时钟
	GPIO_Set(GPIOD,PIN0|PIN14|PIN15,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU);//设置PD0 PD14 PD15
	GPIO_Set(GPIOE,(0xFF<<7),GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU);						//设置PE7~14
	LCD1602_Write_Command(0x38);                 //开显示
	LCD1602_Write_Command(0x0c);                 //开显示但不显示光标
	LCD1602_Write_Command(0x06);                 //写一个字符后地址自动加一
	LCD1602_Write_Command(0x01);                 //清屏
}

/*****************************************************************************
函数：LCD1602显示字符串函数
输入：x    列开始的位置 0~15
			y    行位置
					y=1第一行
					y=2第二行
			*p    字符数组（指针）
            lenth 字符长度
输出：无
作用：在LCD1602显示字符串。
******************************************************************************/
void LCD1602_Display_String(u8 x,u8 y,const u8 *p,u8 length)
{
	u8 address;
	if(y==1)
		address=0x80+x;
	else
		address=0xC0+x;
	LCD1602_Write_Command(address);
	while(length)
	{
		LCD1602_Write_Data(*p);
		p++;
		length--;
	}
}

/*****************************************************************************
函数：LCD1602显示单个字符函数
输入：x    列开始的位置 0~15
			y    行位置
					y=1第一行
					y=2第二行
			cha  输入的字符
输出：无
作用：在LCD1602显示字符。
******************************************************************************/
void LCD1602_Display_Char(u8 x,u8 y,u8 cha)
{
	u8 address;
	if(y==1)
		address=0x80+x;
	else
		address=0xC0+x;
	LCD1602_Write_Command(address);
	LCD1602_Write_Data(cha);
}

/*****************************************************************************
函数：LCD1602显示数字函数
输入：x    列开始的位置 0~15
			y    行位置
					y=1第一行
					y=2第二行
			num   显示的数字0~65535
输出：无
作用：在LCD1602显示字符串。
******************************************************************************/
void LCD1602_Display_Num(u8 x,u8 y,u16 num)
{
	u8 address;
	u8 i;
	u16 temp=1;
	u8 display_num[16];
	u8 length;
	//给数字定长度
	if(num>9999)		length=5;
	else if(num>999)		length=4;
	else if(num>99)		length=3;
	else if(num>9)	length=2;
	else	length=1;
	//转换成数组
	for(i=length;i>1;i--)
		temp=temp*10;
	if(length>=2)
	{
		display_num[length-1]=num/temp;
		for(i=length-2;i>0;i--)
		{
			display_num[i]=num%temp/(temp/10);
			temp=temp/10;
		}
	}
	display_num[0]=num%10;
	
	if(y==1)
		address=0x80+x;
	else
		address=0xC0+x;
	LCD1602_Write_Command(address);

	for(i=length;i>0;i--)
		LCD1602_Write_Data(display_num[i-1]+48);

}

/*****************************************************************************
函数：LCD1602清空行函数
输入：y  行数
输出：无
作用：清空该行内容。
******************************************************************************/
void LCD1602_Clear_Line(u8 y)
{
	u8 i;
	if(y==1)
		LCD1602_Write_Command(0x80);
	else
		LCD1602_Write_Command(0xC0);
	for(i=0;i<16;i++)
		LCD1602_Write_Data(' ');
}

/*****************************************************************************
函数：LCD1602显示菜单函数
输入：无
输出：无
作用：通过运行本函数的次数来判断显示的菜单。
******************************************************************************/
//菜单状态，0显示转向和工作状态，1显示转速环Kp、Ki，2显示设定转速
void LCD1602_Display_Menu(u8 menu)
{
	LCD1602_Display_String(0,1,"RPM:",4);
	LCD1602_Display_String(14,1,"mA",2);
	LCD1602_Clear_Line(2);
	switch(menu)
	{
		case(0):
		{
			LCD1602_Display_String(0,2,"SIDE:",5);
			LCD1602_Display_String(9,2,"STA:",4);
			update|=3<<3;
			break;
		}
		case(1):
		{
			LCD1602_Display_String(0,2,"Knp:",4);
			LCD1602_Display_Num(4,2,Knp);
			LCD1602_Display_String(8,2,"Kni:",4);
			LCD1602_Display_Num(12,2,Kni);
			break;
		}
		case(2):
		{
			LCD1602_Display_String(0,2,"Set Speed:",10);
			update|=1<<6;
			break;
		}
	}
}
