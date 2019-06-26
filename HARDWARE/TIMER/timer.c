#include "timer.h"
#include "sys.h"
#include "led.h"
/**************************************************************
开发板              PWM
PB2         -->      PB2
**************************************************************/
//修改说明
//新增TIM3_PWM_Init函数,用于PWM输出

/*******************************************
以下为TIM3产生PWM的代码
*******************************************/
TIM_HandleTypeDef TIM3_Handler;        		//定时器句柄 
TIM_OC_InitTypeDef TIM3_CH4Handler;       //定时器3通道4句柄

//TIM3 PWM部分初始化 
//PWM输出初始化
//arr：自动重装值
//psc：时钟预分频数
void TIM3_PWM_Init(u16 arr,u16 psc)
{ 
    TIM3_Handler.Instance=TIM3;            											//定时器3
    TIM3_Handler.Init.Prescaler=psc;       											//定时器分频
    TIM3_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;		//向上计数模式
    TIM3_Handler.Init.Period=arr;          												//自动重装载值
    TIM3_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&TIM3_Handler);       										//初始化PWM
    
    TIM3_CH4Handler.OCMode=TIM_OCMODE_PWM1;					 //模式选择PWM1
    TIM3_CH4Handler.Pulse=arr/2;            											//设置比较值,此值用来确定占空比，
																											//默认比较值为自动重装载值的一半,即占空比为50%
    TIM3_CH4Handler.OCPolarity=TIM_OCPOLARITY_LOW;			//输出比较极性为低 
    HAL_TIM_PWM_ConfigChannel(&TIM3_Handler,&TIM3_CH4Handler,TIM_CHANNEL_4);//配置TIM3通道4
    HAL_TIM_PWM_Start(&TIM3_Handler,TIM_CHANNEL_4);			//开启PWM通道4
}

//定时器底层驱动，时钟使能，引脚配置
//此函数会被HAL_TIM_PWM_Init()调用
//htim:定时器句柄
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_TIM3_CLK_ENABLE();											//使能定时器3
    __HAL_RCC_GPIOB_CLK_ENABLE();										//开启GPIOB时钟
	
    GPIO_Initure.Pin=GPIO_PIN_1;           											//PB1
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;  									//复用推完输出
    GPIO_Initure.Pull=GPIO_PULLUP;         										//上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     								//高速
	GPIO_Initure.Alternate=GPIO_AF2_TIM3;										//PB1复用为TIM3_CH4
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
}

/*******************************************
以下为TIM4LED闪烁的代码
*******************************************/
TIM_HandleTypeDef TIM4_Handler;
//通用定时器4中断初始化
//arr:自动重装初值
//psc:时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft (us)
//Ft定=时器器工作频率，单位:MHz
//这里使用的是定时器4，挂在APB1上，时钟为HCLK/2

void TIM4_Init(u16 arr,u16 psc)
{
	TIM4_Handler.Instance=TIM4;														//通用定时器4
	TIM4_Handler.Init.Prescaler=psc;													//分频
	TIM4_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;		//向上计数器
	TIM4_Handler.Init.Period=arr;															//自动装载值
	TIM4_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;	//时钟分频因数
	HAL_TIM_Base_Init(&TIM4_Handler);
	HAL_TIM_Base_Start_IT(&TIM4_Handler);									//使能定时器4和定时器4更新中断
}

//定时器底层驱动，开始时钟，设置中断优先级
//此函数会被HAL_TIM_Base_Init()函数调用
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	__HAL_RCC_TIM4_CLK_ENABLE();											//使能TIM4时钟
	HAL_NVIC_SetPriority(TIM4_IRQn,5,0);										//设置中断优先级，抢占优先级5
	HAL_NVIC_EnableIRQ(TIM4_IRQn);												//开启TIM4中断
}

//定时器4中断服务函数
void TIM4_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM4_Handler);
}

/*******************************************
以下为TIM5输入捕获的代码
*******************************************/
TIM_HandleTypeDef TIM5_Handler;         //定时器5句柄

//定时器5通道1输入捕获配置
//arr 自动重装值
//psc 时钟预分频数
void TIM5_CH1_Cap_Init(u32 arr,u16 psc)
{  
    TIM_IC_InitTypeDef TIM5_CH1Config;  
    
    TIM5_Handler.Instance=TIM5;													//通用定时器5使能
    TIM5_Handler.Init.Prescaler=psc;												//分频数
    TIM5_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;	//向上计数器
    TIM5_Handler.Init.Period=arr;														//自动重装值
    TIM5_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&TIM5_Handler);
    
    TIM5_CH1Config.ICPolarity=TIM_ICPOLARITY_RISING;			//上升沿捕获
    TIM5_CH1Config.ICSelection=TIM_ICSELECTION_DIRECTTI;//映射到TI1上
    TIM5_CH1Config.ICPrescaler=TIM_ICPSC_DIV1;					//配置输入分频：不分频
    TIM5_CH1Config.ICFilter=0;														//配置滤波器：不滤波
    HAL_TIM_IC_ConfigChannel(&TIM5_Handler,&TIM5_CH1Config,TIM_CHANNEL_1);//配置TIM5通道1
    HAL_TIM_IC_Start_IT(&TIM5_Handler,TIM_CHANNEL_1);		//开始捕获TIM5的通道1
    __HAL_TIM_ENABLE_IT(&TIM5_Handler,TIM_IT_UPDATE);	//使能更新中断
}

//定时器5底层驱动，时钟使能，引脚配置
//此函数会被HAL_TIM_IC_Init()调用
//htim：定时器5句柄
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_TIM5_CLK_ENABLE();										//使能TIM5时钟
    __HAL_RCC_GPIOH_CLK_ENABLE();									//开启GPIOH时钟
	
    GPIO_Initure.Pin=GPIO_PIN_10;												//PH10
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;								//复用推挽输出
    GPIO_Initure.Pull=GPIO_PULLDOWN;										//下拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;								//高速
    GPIO_Initure.Alternate=GPIO_AF2_TIM5;									//PH10复用为TIM5通道1
    HAL_GPIO_Init(GPIOH,&GPIO_Initure);

    HAL_NVIC_SetPriority(TIM5_IRQn,2,0);									//抢占优先级2
    HAL_NVIC_EnableIRQ(TIM5_IRQn);											//开启TIM5中断
}

//[7]:0:没有成功的捕获;1:成功捕获到一次.
//[6]:0:还没捕获到低电平;1:已经捕获到低电平了.
//[5:0]:捕获低电平后溢出的次数(对于32位定时器来说,1us计数器加1,溢出时间:4294秒)
u8  TIM5CH1_CAPTURE_STA=0;	//输入捕获状态		    				
u32	TIM5CH1_CAPTURE_VAL;	//输入捕获值(TIM2/TIM5是32位)

//定时器5中断服务函数
void TIM5_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM5_Handler);//定时器共用处理函数
}
 
//定时器输入捕获中断处理回调函数，该函数在HAL_TIM_IRQHandler中会被调用
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)//捕获中断发生时执行
{
	if((TIM5CH1_CAPTURE_STA&0x80)==0)//还未成功捕获
	{
		if(TIM5CH1_CAPTURE_STA&0x40)		//捕获到下一个上升沿 		
		{	  			
			TIM5CH1_CAPTURE_STA|=0x80;		//标记成功捕获到一次高电平和相邻的一次低电平脉宽
			TIM5CH1_CAPTURE_VAL=HAL_TIM_ReadCapturedValue(&TIM5_Handler,TIM_CHANNEL_1);//获取当前的捕获值.
			TIM_RESET_CAPTUREPOLARITY(&TIM5_Handler,TIM_CHANNEL_1);   //一定要先清除原来的设置！！
		}
		else  								//还未开始,第一次捕获上升沿
		{
			TIM5CH1_CAPTURE_STA=0;			//清空
			TIM5CH1_CAPTURE_VAL=0;
			TIM5CH1_CAPTURE_STA|=0x40;		//标记捕获到了上升沿
			__HAL_TIM_DISABLE(&TIM5_Handler);        //关闭定时器5
			__HAL_TIM_SET_COUNTER(&TIM5_Handler,0);//清空定时器5计数寄存器
			TIM_RESET_CAPTUREPOLARITY(&TIM5_Handler,TIM_CHANNEL_1);   //一定要先清除原来的设置！！
			__HAL_TIM_ENABLE(&TIM5_Handler);//使能定时器5
		}		    
	}	
}

/*定时器总中断回调函数（包括了定时器4和定时器5）*/
//定时器更新中断（计数溢出）中断处理回调函数， 该函数在HAL_TIM_IRQHandler中会被调用
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)//更新中断（溢出）发生时执行
{
	/*如果是定时4计数溢出*/
	if(htim==(&TIM4_Handler))
	{
		LED1(1);
		__HAL_TIM_SET_COUNTER(&TIM4_Handler,0);
		HAL_TIM_Base_Stop_IT(&TIM4_Handler);
	}
	/*如果是定时器5计数溢出*/
	if(htim==(&TIM5_Handler))
	{
		if((TIM5CH1_CAPTURE_STA&0x80)==0)//还未成功捕获	1000 0000
		{
			if(TIM5CH1_CAPTURE_STA&0x40)//已经捕获到高电平了	0100 0000
			{
				if((TIM5CH1_CAPTURE_STA&0x3F)==0x3F)//高电平太长了	0011 1111
				{
					TIM5CH1_CAPTURE_STA|=0x80;		//标记成功捕获了一次
					TIM5CH1_CAPTURE_VAL=0xFFFFFFFF;
				}
				else
					TIM5CH1_CAPTURE_STA++;
			}	 
		}
	}		
}
/*定时器5通道1捕获数值获取函数*/
/*事实上这段代码已经没什么用了*/
u32 get_TIM5_CH1_Cap(void)
{
	u32 capture;
	/***************************************************
	下面这一行代码十分恐怖，只要一加进去，capture的值一被读取就会变0。在仿真中可
	以看到，TIM5CH1_CAPTURE_STA的值并不是简单的uchar值，而是在原本uchar值的后
	面多了一个问号和一个乱码，其他uchar变量都不会出现这种问题。——崔
	***************************************************/
	//capture=TIM5CH1_CAPTURE_STA&0x3F;
	//capture*=0xFFFFFFFF;
	capture+=TIM5CH1_CAPTURE_VAL;//得到两个上升沿相邻的时间
	//capture=capture/1000000;
	/***************************************************
	下面的这段if代码是为了解决屏蔽TIM5CH1_CAPTURE_STA后，无法检测到低于457rpm以
	下的转速。（后注：其实不加也一样可以检测，不过也是触及了我的认知范围）——崔
	***************************************************/
	//if(TIM5CH1_CAPTURE_STA&0x01)
	//	capture+=65535;
	//rpm=30000000/capture;
	/*
	if((TIM5CH1_CAPTURE_STA&0x3F)>2)
		rpm=0;
	*/
	return capture;
}
