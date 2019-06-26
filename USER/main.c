/**********************************************
在这里添加包括的头文件（注：崔兆初）
***********************************************/
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timer.h"
#include "adc.h"
#include "key.h"
#include "LCD1602.h"

/*****************************************
在这里开始配置任务（注：崔兆初）
*****************************************/
/*开始任务*/
#define START_TASK_PRIO		1		//任务优先级
#define START_STK_SIZE 		128 		//任务堆栈大小	
TaskHandle_t StartTask_Handler;			//任务句柄
void start_task(void *pvParameters);	//任务函数
/*输出任务：LCD1602显示数据*/
#define Display_TASK_PRIO		2
#define Display_STK_SIZE 		512
TaskHandle_t DisplayTask_Handler;
void Display_task(void *pvParameters);
/*输入任务：接收上位机命令、接收按键*/
#define Input_TASK_PRIO		3
#define Input_STK_SIZE 		512
TaskHandle_t InputTask_Handler;
void Input_task(void *pvParameters);
/*PID任务*/
#define PID_TASK_PRIO		4
#define PID_STK_SIZE 		512  
TaskHandle_t PIDTask_Handler;
void PID_task(void *pvParameters);

/*在这里定义main.c的全局变量,某些工程全局变量定义在sys.h中*/
u8 alpha;						//占空比
u8 last_alpha;				//上次占空比
u8 motor_sta;				//电机状态，bit0停止（0）/启动（1）转动、bit1正转（0）/反转（1）、bit2关闭/开启电流环、bit3强制修改占空比
u8 Knp,Kni;					//转速环比例参数和积分参数
u16 rpm; 					//实时转速
u16 rpm_setting;		//设定转速
u16 adc_value0;			//零电流参考电压
u16 mA_Current;		//电流值（毫安）
u8 update;					//数据更新状态，(bit0更新占空比)、(bit1更新电流)、bit2更新转速、
									//bit3更新电机停止/启动状态、bit4更新转向、bit5更新菜单、bit6
									//更新设定转速、bit7更新PID参数
									
/*************************
主程序（注：崔兆初）
**************************/
int main(void)
{
    Cache_Enable();             	//打开L1-Cache
    HAL_Init();				        	//初始化HAL库
    Stm32_Clock_Init(432,25,2,9);   //设置时钟,216Mhz 
	LED_Init();							//初始化LED
    delay_init(216);                	//延时初始化
	uart_init(912600);				//串口初始化
	KEY_Init();							//初始化按键
	Adc_Init();							//初始化ADC
	LCD1602_Init();					//LCD1602初始化
	GPIO_Set(GPIOB,PIN12,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU);
	//初始化TIM3输出PWM
	//TIM3作为PWM输出定时器，108M/108=1M的计数频率，
	//自动重装载为500，那么PWM频率为1M/500=2KHz
	TIM3_PWM_Init(500-1,108-1);
	PWM_VAL=500;
	
	//TIM5以1MHz频率计数
	TIM5_CH1_Cap_Init(0XFFFFFFFF,108-1); 
	
	rpm_setting=6000;			//定义设定转速为6k转
	
	//定时器4定时250ms，中断后，关闭定时器4使能和中断以及关闭LED1，代表初始化完成
	TIM4_Init(2500-1,10800-1);
	
    //创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          			//任务名称
                (uint16_t       )START_STK_SIZE,     		    //任务堆栈大小
                (void*          )NULL,                				    //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,    //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);     //任务句柄              
    vTaskStartScheduler();                                          //开启任务调度
}

/***************************************
开始任务任务函数（注：崔兆初）
***************************************/
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
	/*在这里添加任务*/
	//创建输出任务
	xTaskCreate((TaskFunction_t )Display_task,     	
                (const char*    )"Display_task",   	
                (uint16_t       )Display_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )Display_TASK_PRIO,	
                (TaskHandle_t*  )&DisplayTask_Handler);   
	
	//创建输入任务
	xTaskCreate((TaskFunction_t )Input_task,     	
                (const char*    )"Input_task",   	
                (uint16_t       )Input_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )Input_TASK_PRIO,	
                (TaskHandle_t*  )&InputTask_Handler);   
				
    //创建PID任务
    xTaskCreate((TaskFunction_t )PID_task,     	
                (const char*    )"PID_task",   	
                (uint16_t       )PID_STK_SIZE, 
                (void*          )NULL,				
                (UBaseType_t    )PID_TASK_PRIO,	
                (TaskHandle_t*  )&PIDTask_Handler);   
	
	/*截止添加任务*/
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            		//退出临界区
}

//PID任务函数 
void PID_task(void *pvParameters)
{
	static u8 times;					//读取转速值不变次数
	u16 last_rpm;						//上次读取转速
	double effect,last_effect;	//输入偏差以及上次输入偏差
	double dUn;						//转速环输出偏差
	double Un;							//转速环输出
	double last_Un;					//上次转速环输出
	while(1)
	{
		/*停转后，TIM5CH1不会中断，即不会更新转速，需要程序介入修改0转速*/
		if(last_rpm==rpm)
		{
			times++;
			if(times==10)										//如果转速不变10次，则认为是停转了
			{
				update|=0x04;									//更新信息
				rpm=0;
				/*发送0转速*/
				while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
				USART1->TDR=0xF5;
				/*发送0转速完毕*/
			}
		}
		else
		{
			last_rpm=rpm;
			times=0;
		}
		/*读取转速*/
		if(TIM5CH1_CAPTURE_STA&0x80)		//计数完成
		{
			taskENTER_CRITICAL();         		  //进入临界区
			rpm=30000000/TIM5CH1_CAPTURE_VAL;//计算转速
			if(rpm>8000)
				rpm=last_rpm;
			if(last_rpm-rpm>3000)
				rpm=last_rpm;
			/*发送转速信息*/
			while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
			USART1->TDR=0xF6;
			while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
			USART1->TDR=rpm/1000;
			while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
			USART1->TDR=rpm%1000/100;
			while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
			USART1->TDR=rpm%100/10;
			while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
			USART1->TDR=rpm%10;
			/*发送转速信息完毕*/
			TIM5CH1_CAPTURE_STA=0;
			taskEXIT_CRITICAL();        		  				//退出临界区
			update|=0x04;												//更新信息
		}
		/*PID算法*/
		if(motor_sta&0x01)										//若电机为启动状态
		{
			taskENTER_CRITICAL();     	  				    //进入临界区
			last_effect=effect;										//保存上次输入偏差
			effect=rpm_setting-rpm;								//计算输入偏差
			/*比例算法*/
			dUn=-Knp*(effect-last_effect)/10000;
			/*积分算法*/
			dUn=dUn+Kni*effect/10000;
			Un=dUn+last_Un;										//PID增量加上上次PID输出值
			last_Un=Un;												//保存上次PID输出值
			/*限幅*/
			if(Un<0)
				Un=0;
			if(Un>100)
				Un=100;
			/*修改PWM的值，由实验数据得到，PWM_VAL在0~100（12V到7V）为最佳调速区*/
			PWM_VAL=100-Un;
			taskEXIT_CRITICAL();         					   //退出临界区
		}
		vTaskDelay(100);											//任务延时
	}
}   

//输入任务函数 
void Input_task(void *pvParameters)
{
	u8 key;
	u8 temp[16];
	while(1)
	{
		/*串口接收*/
		if(USART_RX_STA&0x8000)								//如果串口接受完数据
		{
			HAL_TIM_Base_Start_IT(&TIM4_Handler);		//使能TIM4，中断后关闭使能及LED1
			LED1(0);																//开启LED1
			switch(USART_RX_BUF[0])
			{
				/*修改PWM_VAL，仅调试用*/
				case(0xAF):
				{
					PWM_VAL=USART_RX_BUF[1]*100+USART_RX_BUF[2]*10+USART_RX_BUF[3];
					break;
				}
				/*修改转速环比例参数*/
				case(0xA0):
				{
					Knp=USART_RX_BUF[1];
					update|=1<<7;
					break;
				}
				/*修改转速环微分参数*/
				case(0xA1):
				{
					Kni=USART_RX_BUF[1];
					update|=1<<7;
					break;
				}
				/*修改运行状态*/
				case(0xA3):
				{
					if(USART_RX_BUF[1])//如果第二个字节为1
						motor_sta+=1;		//运行
					else							//否则
						motor_sta-=1;			//停止
					update|=1<<3;
					PWM_VAL=500;
					break;
				}
				/*修改运行方向*/
				case(0xA4):
				{
					if(USART_RX_BUF[1])//如果第二个字节为1
					{
						motor_sta+=2;			//反向
						motor_side(0);
					}
					else								//否则
					{
						motor_sta-=2;				//正向
						motor_side(1);
					}
					update|=1<<4;
					break;
				}
				/*修改设定转速*/
				case(0xA5):
				{
					rpm_setting=USART_RX_BUF[1]*1000+USART_RX_BUF[2]*100+USART_RX_BUF[3]*10+USART_RX_BUF[4];
					update|=1<<6;
					break;
				}
				/*重新设定0电流*/
				case(0xAA):
				{
					adc_value0=Get_Adc_Average(5,20);
					break;
				}
				/*STM32上传信息到上位机*/
				case(0xA8):
				{
					temp[0]=0xF4;						//command
					temp[1]=motor_sta&0x01;		//on_off
					temp[2]=motor_sta&0x02;		//side
					temp[3]=rpm_setting/1000;
					temp[4]=rpm_setting%1000/100;
					temp[5]=rpm_setting%100/10;
					temp[6]=rpm_setting%10;
					temp[7]=Knp;
					temp[8]=Kni;
					taskENTER_CRITICAL();         		  //进入临界区
					for(temp[15]=0;temp[15]<9;temp[15]++)
					{
						while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
						USART1->TDR=temp[temp[15]];
					}
					taskEXIT_CRITICAL();        		  				//退出临界区
					break;
				}
			}
			USART_RX_STA=0;										  //清空串口接收状态
		}
		/*按键扫描*/
		key=KEY_Scan(0);                            				  		//按键扫描
		switch(key)
		{
			/*key0（最右侧按键）按下，启动/停止转动*/
			case(1):
			{
				HAL_TIM_Base_Start_IT(&TIM4_Handler);		//使能TIM4，中断后关闭使能及LED1
				LED1(0);																//开启LED1
				if(motor_sta&0x01)											//motor_sta bit0 为1时，启动->停止
				{
					motor_sta-=1;													//motor_sta bit0 置0，停止
					PWM_VAL=500;												//PWM输出为0
					/*发送停转信息*/
					while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
					USART1->TDR=0xF0;
				}
				else
				{
					motor_sta+=1;												//motor_sta bit0 置1
					/*发送启动信息*/
					while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
					USART1->TDR=0xF1;
				}
				update|=1<<3;													//更新信息
				break;
			}
			/*key1按下（中间按键），记录零电流参考电压*/
			case(2):
			{
				HAL_TIM_Base_Start_IT(&TIM4_Handler);
				LED1(0);
				adc_value0=Get_Adc_Average(5,20);				//获取零电流参考电压
				break;
			}
			/*key2按下（最左侧按键），切换菜单*/
			case(3):
			{
				HAL_TIM_Base_Start_IT(&TIM4_Handler);
				LED1(0);
				update|=1<<5;
				break;
			}
			/*key_up按下，切换正反转*/
			case(4):
			{
				HAL_TIM_Base_Start_IT(&TIM4_Handler);
				LED1(0);
				if(motor_sta&0x02)											//motor_sta bit1 为1时，反转->正转
				{
					motor_side(0);
					motor_sta-=2;													//motor_sta bit1 置0
					/*发送反转信息*/
					while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
					USART1->TDR=0xF2;
				}
				else
				{
					motor_side(1);
					motor_sta+=2;												//motor_sta bit1 置1
					/*发送正转信息*/
					while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
					USART1->TDR=0xF3;
				}
				update|=1<<4;
				break;
			}
		}
		vTaskDelay(200);
	}
}   

//显示任务函数 
void Display_task(void *pvParameters)
{
	u16 adc_value;						//adc读取值
	float current;							//电流值
	static u8 menu=0;					//菜单状态，0显示转向和工作状态，1显示转速环Kp、Ki，2显示设定转速
	/*显示菜单文本*/
	LCD1602_Display_String(0,1,"RPM:",4);
	LCD1602_Display_String(9,1,"mA:",3);
	LCD1602_Display_Menu(menu);
	LCD1602_Display_String(13,2,"OFF",3);
	LCD1602_Display_String(5,2,"L",1);
	adc_value0=Get_Adc_Average(5,20);//获取零电流参考电压
	while(1)
	{
		/*更新显示菜单*/
		if(update&0x20)
		{
			menu++;
			if(menu==3)
				menu=0;
			LCD1602_Display_Menu(menu);		//显示对应菜单
			LCD1602_Display_String(9,1,"mA:",3);
			update-=32;										//已经响应了更新
		}
		/*更新显示电流*/
		adc_value=Get_Adc_Average(5,20);							//获取测量电压值
		current=(adc_value-adc_value0)*3.3/(4096*0.185);	//计算电流值
		if(current<0)																//若电流为负值，则取反
			current*=-1;
		mA_Current=current*1000;										//化为毫安
		LCD1602_Display_Num(12,1,mA_Current);				//显示毫安电流值
		if(mA_Current<1000)													//清空多余显示位置
			LCD1602_Write_Data(' ');
		if(mA_Current<100)
			LCD1602_Write_Data(' ');
		if(mA_Current<10)
			LCD1602_Write_Data(' ');
		/*更新显示转速*/
		if(update&0x04)															//若需要更新显示
		{
			LCD1602_Display_Num(4,1,rpm);							//显示转速
			LCD1602_Write_Data(' ');										//清空多余显示位置
			if(rpm<1000)
				LCD1602_Write_Data(' ');
			if(rpm<100)
				LCD1602_Write_Data(' ');
			if(rpm<10)
				LCD1602_Write_Data(' ');
			update-=4;																//已经响应了更新
		}
		/*更新电机停止/启动状态*/
		if(update&0x08&&menu==0)										//若需要更新显示
		{
			if(motor_sta&0x01)												//motor_sta bit0 为1时，启动
			{
				LCD1602_Display_String(13,2,"ON",2);			//显示ON
				LCD1602_Write_Data(' ');									//清除显示OFF时的最后一个F
			}
			else																		//停止
				LCD1602_Display_String(13,2,"OFF",3);			//显示OFF
			update-=8;																//已经响应了更新
		}
		/*更新转向*/
		if(update&0x10&&menu==0)										//若需要更新显示
		{
			if(motor_sta&0x02)												//motor_sta bit1 为1时，反转
				LCD1602_Display_String(5,2,"R",1);
			else																		//正转
				LCD1602_Display_String(5,2,"L",1);
			update-=16;															//已经响应了更新
		}
		/*更新显示设定转速*/
		if(update&0x40&&menu==2)										//若需要更新显示
		{
			LCD1602_Display_Num(10,2,rpm_setting);			//显示设定转速
			if(rpm_setting<1000)												//清空多余显示位
				LCD1602_Write_Data(' ');
			update-=64;															//已经响应了更新
		}
		/*更新显示PID参数*/
		if(update&0x80&&menu==1)
		{
			LCD1602_Display_Menu(1);
			update-=128;															//已经响应了更新
		}
		vTaskDelay(500);
	}
}   
