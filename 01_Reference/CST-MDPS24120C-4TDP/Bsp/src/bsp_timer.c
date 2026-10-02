/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_timer.c
 *
 * @par dependencies
 * - bsp_timer.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the timer
 *
 *	Implemented multiple software timers for the main program (accuracy: 1ms), 
 *	the number of timers can be increased or decreased by modifying TMR_COUNT.
 *	Implemented millisecond-level delay functions (accuracy: 1ms) and microsecond-level delay functions.
 *	Implemented system runtime function (unit: 1ms).
 *	Implemented TIM (optional) hardware timer interrupt for microsecond-level timing.
 *
 * @version    V1.0     2025-09-08      ALan
 * 			   V1.1     2025-10-22      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/

 //******************************** Includes *********************************//
#include "bsp_timer.h"
//******************************** Includes *********************************//


//******************************** Defines **********************************//

////prog trig
static void (*s_TIM2_CallBack1)(void);
static void (*s_TIM2_CallBack2)(void);
static void (*s_TIM2_CallBack3)(void);
static void (*s_TIM2_CallBack4)(void);
////for some mission
static void (*s_TIM4_CallBack1)(void);
static void (*s_TIM4_CallBack2)(void);
static void (*s_TIM4_CallBack3)(void);
static void (*s_TIM4_CallBack4)(void);
////for cam out
static void (*s_TIM1_CallBack1)(void);
static void (*s_TIM1_CallBack2)(void);
static void (*s_TIM1_CallBack3)(void);
static void (*s_TIM1_CallBack4)(void);


/* These two global variables are used for the bsp_DelayMS() function */
static volatile uint32_t s_uiDelayCount = 0;
static volatile uint8_t s_ucTimeOutFlag = 0;
static volatile uint8_t g_1ms_flag		= 0;
static volatile uint8_t g_10ms_flag		= 0;

/* Define software timer struct variable */
static SOFT_TMR s_tTmr[TMR_COUNT] = {0};

/*
   Global runtime, unit: 1ms
   Maximum can represent 24.85 days, overflow must be considered if product runs longer
*/
__IO int32_t g_iRunTime = 0;

static __IO uint8_t g_ucEnableSystickISR = 0;	/* Wait for variable initialization */

static void bsp_SoftTimerDec(SOFT_TMR *_tmr);
//******************************** Defines **********************************//


/**
 * @brief Initialize system tick timer and software timer variables
 * @param None
 * @retval None
 */
void bsp_InitTimer(void)
{
	uint8_t i;

	/* Clear all software timers */
	for (i = 0; i < TMR_COUNT; i++)
	{
		s_tTmr[i].Count = 0;
		s_tTmr[i].PreLoad = 0;
		s_tTmr[i].Flag = 0;
		s_tTmr[i].Mode = TMR_ONCE_MODE;	/* Default is one-shot mode */
	}

	/*
		Configure and enable the SysTick interrupt with a 1 ms period:

		*SystemCoreClock / 1000: 1 ms period (1000 Hz)

		*SystemCoreClock / 500: 2 ms period (500 Hz)

		*SystemCoreClock / 2000: 500 ?s period (2000 Hz)

		1 ms period suits general applications; use 10 ms for low-speed or low-power applications.
    */
	//SysTick_Config(SystemCoreClock / 1000);
	HAL_SYSTICK_Config(SystemCoreClock / 1000);
	
	g_ucEnableSystickISR = 1;		/* 1 means execute SysTick interrupt */
	
	//bsp_InitHardTimer();
}



/**
 * @brief SysTick interrupt service routine, triggered every 1ms
 * @param None
 * @retval None
 */
extern void bsp_RunPer1ms_interrupt(void);
extern void bsp_RunPer10ms_interrupt(void);
void SysTick_ISR(void)
{
	static uint8_t s_count = 0;
	uint8_t i;
	
	/* Enter every 1ms (used only for bsp_DelayMS) */
	if (s_uiDelayCount > 0)
	{
		if (--s_uiDelayCount == 0)
		{
			s_ucTimeOutFlag = 1;
		}
	}

	/* Decrement software timer counters every 1ms */
	for (i = 0; i < TMR_COUNT; i++)
	{
		bsp_SoftTimerDec(&s_tTmr[i]);
	}

	/* Global runtime increases by 1 every 1ms */
	g_iRunTime++;
	if (g_iRunTime == 0x7FFFFFFF)	/* int32_t  0x7FFFFFFF */
	{
		g_iRunTime = 0;
	}

	g_1ms_flag = 1;
	bsp_RunPer1ms_interrupt();		

	if (++s_count >= 10)
	{
		s_count = 0;

		g_10ms_flag = 1;
		bsp_RunPer10ms_interrupt();	
	}
}

/**
 * @brief Decrements all timer variables by 1 every 1ms. Must be called periodically by SysTick_ISR.
 * @param _tmr Pointer to the timer variable
 * @return None
 */
static void bsp_SoftTimerDec(SOFT_TMR *_tmr)
{
	if (_tmr->Count > 0)
	{
		/* If timer variable decrements to 1, set timer reached flag */
		if (--_tmr->Count == 0)
		{
			_tmr->Flag = 1;

			/* If auto mode, reload counter automatically */
			if(_tmr->Mode == TMR_AUTO_MODE)
			{
				_tmr->Count = _tmr->PreLoad;
			}
		}
	}
}

/**
 * @brief Millisecond-level delay function. Must be called after SysTick timer starts.
 * @param n Delay length in milliseconds
 * @retval None
 */
void bsp_DelayMS(uint32_t n)
{
	if (n == 0)
	{
		return;
	}
	else if (n == 1)
	{
		n = 2;
	}

	DISABLE_INT();  			/* Disable interrupt */

	s_uiDelayCount = n;
	s_ucTimeOutFlag = 0;

	ENABLE_INT();  				/* Enable interrupt */

	while (1)
	{
		bsp_Idle();				/* CPU idle operation, see bsp.c and bsp.h */

		/*
			Wait for delay time to reach
			Note: Compiler may optimize incorrectly, so s_ucTimeOutFlag must be declared volatile
		*/
		if (s_ucTimeOutFlag == 1)
		{
			break;
		}
	}
}

/**
 * @brief Microsecond-level delay function. Must be called after SysTick timer starts.
 * @param n Delay length in microseconds
 * @retval None
 */
void bsp_DelayUS(uint32_t n)
{
    uint32_t ticks;
    uint32_t told;
    uint32_t tnow;
    uint32_t tcnt = 0;
    uint32_t reload;
       
	reload = SysTick->LOAD;                
    ticks = n * (SystemCoreClock / 1000000);	 /* Number of counts to wait */ 
    
    tcnt = 0;
    told = SysTick->VAL;            /* Current counter value at entry */
    while (1)
    {
        tnow = SysTick->VAL;    
        if (tnow != told)
        {    
			/* SysTick is a decrementing counter */
            if (tnow < told)
            {
                tcnt += told - tnow;    
            }
			/* Reload decrementing */
            else
            {
                tcnt += reload - tnow + told;    
            }        
            told = tnow;

             /* Check if required ticks have been reached */
            if (tcnt >= ticks)
            {
            	break;
            }
        }  
    }
}


/**
 * @brief Start a timer with specified ID and period
 *
 * @param _id Timer ID (must be less than TMR_COUNT)
 * @param _period Timer period in ticks
 *
 * @note This function will:
 *       1. Validate timer ID (halt if invalid)
 *       2. Disable interrupts during configuration
 *       3. Set timer count, reload value, and mode
 *       4. Re-enable interrupts
 * @warning Invalid timer ID will cause infinite loop (watchdog reset)
 */
void bsp_StartTimer(uint8_t _id, uint32_t _period)
{
	if (_id >= TMR_COUNT)
	{
		//BSP_Printf("Error: file %s, function %s()\r\n", __FILE__, __FUNCTION__);
		while(1); /* Parameter error, system hangs and waits for watchdog reset. */
	}

	DISABLE_INT();  						/* Disable interrupt */

	s_tTmr[_id].Count 	= _period;			/* Real-time counter initial value */
	s_tTmr[_id].PreLoad = _period;			/* Counter auto-reload value, only effective in auto mode */
	s_tTmr[_id].Flag 	= 0;				/* Timer reached flag */
	s_tTmr[_id].Mode 	= TMR_ONCE_MODE;	/* One-shot mode */

	ENABLE_INT();  				/* Enable interrupt */
}


/**
 * @brief Start an auto-reload timer with specified period
 * @param _id Timer ID (must be less than TMR_COUNT)
 * @param _period Timer period in ticks
 * @note This function is not thread-safe (uses critical section)
 * @warning Invalid _id will cause system lockup (watchdog reset)
 */
void bsp_StartAutoTimer(uint8_t _id, uint32_t _period)
{
	if (_id >= TMR_COUNT)
	{
		//BSP_Printf("Error: file %s, function %s()\r\n", __FILE__, __FUNCTION__);
		while(1); /* Parameter error, system hangs and waits for watchdog reset. */
	}

	DISABLE_INT();  		/* Disable interrupt */

	s_tTmr[_id].Count 	= _period;		  	/* Real-time counter initial value */
	s_tTmr[_id].PreLoad = _period;			/* Counter auto-reload value, only effective in auto mode */
	s_tTmr[_id].Flag 	= 0;				/* Timer reached flag */
	s_tTmr[_id].Mode 	= TMR_AUTO_MODE;	/* One-shot mode */

	ENABLE_INT();  			/* Enable interrupt */
}

/**
 * @brief Stop the timer with specified ID
 * @param _id Timer ID (must be less than TMR_COUNT)
 * @note This function is not thread-safe (uses critical section)
 * @warning Invalid _id will cause system lockup (watchdog reset)
 */
void bsp_StopTimer(uint8_t _id)
{
	if (_id >= TMR_COUNT)
	{
		//BSP_Printf("Error: file %s, function %s()\r\n", __FILE__, __FUNCTION__);
		while(1); /* Parameter error, system hangs and waits for watchdog reset. */
	}

	DISABLE_INT();  	/* Disable interrupt */

	s_tTmr[_id].Count = 0;				/* 实时计数器初值 */
	s_tTmr[_id].Flag  = 0;				/* 定时时间到标志 */
	s_tTmr[_id].Mode  = TMR_ONCE_MODE;	/* 自动工作模式 */

	ENABLE_INT();  		/* Enable interrupt */
}

/**
 * @brief Check if the timer with specified ID has reached its period
 * @param _id Timer ID (must be less than TMR_COUNT)
 * @retval 1 if timer has reached its period, 0 otherwise
 * @warning Invalid _id will cause system lockup (watchdog reset)
 */
uint8_t bsp_CheckTimer(uint8_t _id)
{
	if (_id >= TMR_COUNT)
	{
		return 0;
	}

	if (s_tTmr[_id].Flag == 1)
	{
		s_tTmr[_id].Flag = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

/**
 * @brief Check if the 1ms task flag is set. If so, clear it and return 1.
 * @retval 1 if flag was set, 0 otherwise.
 */
uint8_t bsp_Check1msTask(void)
{
    if (g_1ms_flag)
    {
        g_1ms_flag = 0;
        return 1;
    }
    return 0;
}


/**
 * @brief Check if the 10ms task flag is set. If so, clear it and return 1.
 * @retval 1 if flag was set, 0 otherwise.
 */
uint8_t bsp_Check10msTask(void)
{
    if (g_10ms_flag)
    {
        g_10ms_flag = 0;
        return 1;
    }
    return 0;
}

/**
 * @brief Get the system runtime in milliseconds
 * @param None
 * @retval Runtime in milliseconds (int32_t)
 * @note This function is thread-safe (uses critical section)
 */
int32_t bsp_GetRunTime(void)
{
	int32_t runtime;

	DISABLE_INT();  	/* Disable interrupt */

	runtime = g_iRunTime;	/* This variable is modified in SysTick interrupt, so needs interrupt protection */

	ENABLE_INT();  		/* Enable interrupt */

	return runtime;
}

/**
 * @brief Check elapsed time since last recorded time
 * @param _LastTime Previous recorded time (from bsp_GetRunTime)
 * @retval Elapsed time in milliseconds
 * @note This function is thread-safe (uses critical section)
 */
int32_t bsp_CheckRunTime(int32_t _LastTime)
{
	int32_t now_time;
	int32_t time_diff;

	DISABLE_INT();  	/* Disable interrupt */

	now_time = g_iRunTime;	/* This variable is modified in SysTick interrupt, so needs interrupt protection */

	ENABLE_INT();  		/* Enable interrupt */
	
	if (now_time >= _LastTime)
	{
		time_diff = now_time - _LastTime;
	}
	else
	{
		time_diff = 0x7FFFFFFF - _LastTime + now_time;
	}

	return time_diff;
}


/**
 * @brief SysTick interrupt handler
 * @param None
 * @retval None
 * @note This function is called by the system when a SysTick interrupt occurs.
 */
void bsp_systick_handler(void)
{
	if (g_ucEnableSystickISR == 0){
		return;
	}
	SysTick_ISR();	
}

//******************************** init timer  ********************************//
void bsp_init_multi_timer(void)
{
#if BSP_SINGLE_HW_TIMER
    bsp_InitTrigDelayTimer(TIM2, TIM2_IRQn, TRIG_DELAY_100NS);
	bsp_InitTrigDelayTimer(TIM4, TIM4_IRQn, TRIG_DELAY_1US);
	bsp_InitTrigDelayTimer(TIM1, TIM1_CC_IRQn, TRIG_DELAY_1US);
#else
#endif
}


#if STM32
void bsp_InitTrigDelayTimer(TIM_TypeDef *TIMx, IRQn_Type s_TIM_IRQ , uint8_t t_delay)
#else
void bsp_InitTrigDelayTimer(uint32_t timer_periph, IRQn_Type s_TIM_IRQ , uint8_t t_delay)
#endif
{
#if STM32
	TIM_HandleTypeDef TimHandle = {0};
#else
    timer_parameter_struct		timer_initpara;
#endif
	uint32_t usPeriod;
	uint16_t usPrescaler;
	uint32_t uiTIMxCLK;
	
#if STM32

	if( TIMx == TIM4 )
	{
		__HAL_RCC_TIM4_CLK_ENABLE();
	}

	if( TIMx == TIM1 )
	{
		__HAL_RCC_TIM1_CLK_ENABLE();
	}

	if( TIMx == TIM2 )
	{
		__HAL_RCC_TIM2_CLK_ENABLE();
	}

	if( TIMx == TIM8 )
	{
		__HAL_RCC_TIM8_CLK_ENABLE();
	}
#else		
	if( timer_periph == TIMER3 ){
		rcu_periph_clock_enable(RCU_TIMER3);
	}
	
	if( timer_periph == TIMER4 ){
		rcu_periph_clock_enable(RCU_TIMER4);
	}

	if( timer_periph == TIMER1 ){
		rcu_periph_clock_enable(RCU_TIMER1);
	}
#endif

	/* 定时器时钟 */
	uiTIMxCLK = SystemCoreClock ;

	if( t_delay == TRIG_DELAY_1US)
		usPrescaler = uiTIMxCLK / 1000000- 1;	/* 分频比 = 1 */

	if( t_delay == TRIG_DELAY_10US)
		usPrescaler = uiTIMxCLK / 100000 - 1;

	if( t_delay == TRIG_DELAY_100US)
		usPrescaler = uiTIMxCLK / 10000  - 1;

	if( t_delay == TRIG_DELAY_100NS)
		usPrescaler = uiTIMxCLK / 10000000- 1;	

#if STM32
	if (TIMx == TIM2 )
	{
		usPeriod = 0xFFFFFFFF;
	}
	else
	{
		usPeriod = 0xFFFF;
	}
#elif GD32
	usPeriod = 0xFFFF;
#endif

// After setting the prescaler to usPrescaler, the timer counter increments every 1 microsecond
// The parameter usPeriod determines the maximum count value:
// usPeriod = 0xFFFF means a maximum of 0xFFFF microseconds

#if STM32
	TimHandle.Instance 				 = TIMx;
	TimHandle.Init.Prescaler         = usPrescaler;
	TimHandle.Init.Period            = usPeriod;
	TimHandle.Init.ClockDivision     = 0;
	TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	TimHandle.Init.RepetitionCounter = 0;
    TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	
	if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
	{
		Error_Handler();
	}
	
#else
	timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = usPrescaler;
	timer_initpara.period            = usPeriod;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

	timer_init(timer_periph,&timer_initpara);
#endif
	
	//Configure timer interrupt for Capture/Compare (CC) interrupt
#if STM32
	if( TIMx == TIM8 ){
		HAL_NVIC_SetPriority(s_TIM_IRQ, 2, 1);
		HAL_NVIC_EnableIRQ(s_TIM_IRQ);	
	}
	if( TIMx == TIM4 ){
		HAL_NVIC_SetPriority(s_TIM_IRQ, 0, 2);
		HAL_NVIC_EnableIRQ(s_TIM_IRQ);	
	}
	if( TIMx == TIM1 ){
		HAL_NVIC_SetPriority(s_TIM_IRQ, 0, 2);
		HAL_NVIC_EnableIRQ(s_TIM_IRQ);	
	}
	if(TIMx == TIM2 ){
		HAL_NVIC_SetPriority(s_TIM_IRQ, 0, 1);
		HAL_NVIC_EnableIRQ(s_TIM_IRQ);	
	}
	HAL_TIM_Base_Start(&TimHandle);
#else	
	if( timer_periph == TIMER3 ){
		nvic_irq_enable(s_TIM_IRQ,2,1);
	}
	
	if( timer_periph == TIMER4 ){
		nvic_irq_enable(s_TIM_IRQ,2,1);
	}
	if( timer_periph == TIMER1 ){
		nvic_irq_enable(s_TIM_IRQ,2,2);
	}
	timer_auto_reload_shadow_enable(timer_periph);
	timer_enable(timer_periph);
#endif
}


//******************************** start timer  ********************************//
void bsp_tim2_start_cc(uint8_t cc, uint32_t delta_us, void *cb)
{
    if (cc < 1 || cc > 4 || cb == NULL) return;

    TIM_TypeDef *TIMx = TIM2;
    uint32_t now = TIMx->CNT;
    uint32_t target = now + delta_us;

    switch(cc)
    {
        case 1: 
			s_TIM2_CallBack1 = (void(*)(void))cb; 
			TIMx->CCR1 = target; TIMx->SR &= ~TIM_SR_CC1IF; 
			TIMx->DIER |= TIM_DIER_CC1IE; 
			break;
        case 2: 
			s_TIM2_CallBack2 = (void(*)(void))cb; 
			TIMx->CCR2 = target; TIMx->SR &= ~TIM_SR_CC2IF; 
			TIMx->DIER |= TIM_DIER_CC2IE; 
			break;
        case 3: 
			s_TIM2_CallBack3 = (void(*)(void))cb; 
			TIMx->CCR3 = target; TIMx->SR &= ~TIM_SR_CC3IF; 
			TIMx->DIER |= TIM_DIER_CC3IE; 
			break;
        case 4: 
			s_TIM2_CallBack4 = (void(*)(void))cb; 
			TIMx->CCR4 = target; TIMx->SR &= ~TIM_SR_CC4IF; 
			TIMx->DIER |= TIM_DIER_CC4IE; 
			break;

        default: 
		break;
    }
}

void bsp_tim4_start(uint8_t cc, uint32_t delta_us, void *cb)
{
    if (cc < 1 || cc > 4 || cb == NULL) return;

    TIM_TypeDef *TIMx = TIM4;
    uint32_t now = TIMx->CNT;
    uint32_t target = now + delta_us;

    switch(cc)
    {
        case 1: 
			s_TIM4_CallBack1 = (void(*)(void))cb; 
			TIMx->CCR1 = target; TIMx->SR &= ~TIM_SR_CC1IF; 
			TIMx->DIER |= TIM_DIER_CC1IE; 
			break;
        case 2: 
			s_TIM4_CallBack2 = (void(*)(void))cb; 
			TIMx->CCR2 = target; TIMx->SR &= ~TIM_SR_CC2IF; 
			TIMx->DIER |= TIM_DIER_CC2IE; 
			break;
        case 3: 
			s_TIM4_CallBack3 = (void(*)(void))cb; 
			TIMx->CCR3 = target; TIMx->SR &= ~TIM_SR_CC3IF; 
			TIMx->DIER |= TIM_DIER_CC3IE; 
			break;
        case 4: 
			s_TIM4_CallBack4 = (void(*)(void))cb; 
			TIMx->CCR4 = target; TIMx->SR &= ~TIM_SR_CC4IF; 
			TIMx->DIER |= TIM_DIER_CC4IE; 
			break;

        default: 
		break;
    }
}

void bsp_tim1_start(uint8_t cc, uint32_t delta_us, void *cb)
{
    if (cc < 1 || cc > 4 || cb == NULL) return;

    TIM_TypeDef *TIMx = TIM1;
    uint32_t now = TIMx->CNT;
    uint32_t target = now + delta_us;

    switch(cc)
    {
        case 1: 
			s_TIM1_CallBack1 = (void(*)(void))cb; 
			TIMx->CCR1 = target; TIMx->SR &= ~TIM_SR_CC1IF; 
			TIMx->DIER |= TIM_DIER_CC1IE; 
			break;
        case 2: 
			s_TIM1_CallBack2 = (void(*)(void))cb; 
			TIMx->CCR2 = target; TIMx->SR &= ~TIM_SR_CC2IF; 
			TIMx->DIER |= TIM_DIER_CC2IE; 
			break;
        case 3: 
			s_TIM1_CallBack3 = (void(*)(void))cb; 
			TIMx->CCR3 = target; TIMx->SR &= ~TIM_SR_CC3IF; 
			TIMx->DIER |= TIM_DIER_CC3IE; 
			break;
        case 4: 
			s_TIM1_CallBack4 = (void(*)(void))cb; 
			TIMx->CCR4 = target; TIMx->SR &= ~TIM_SR_CC4IF; 
			TIMx->DIER |= TIM_DIER_CC4IE; 
			break;

        default: 
		break;
    }
}



void bsp_start_strobe5_8_timer(uint8_t _CC, uint32_t _uiTimeOut, void * _pCallBack)
{
	
	//uint32_t cnt_now;
    //uint32_t cnt_tar;
#if GD32
	uint32_t timerx = TIM_STROBE_5_8;
    
    cnt_now = TIMER_CNT(timerx);
    cnt_tar = cnt_now + _uiTimeOut;			//计算捕获的计数器值 
    if (_CC == 1)
    {
        s_TIM4_CallBack1 = (void (*)(void))_pCallBack;

		TIMER_CH0CV(timerx) 	=					  cnt_tar; 	
        TIMER_INTF(timerx)  	= (uint16_t)~TIMER_INTF_CH0IF;   
		TIMER_DMAINTEN(timerx) |= 			 TIMER_INTF_CH0IF;	
	}
    else if (_CC == 2)
    {
		s_TIM4_CallBack2 = (void (*)(void))_pCallBack;

		TIMER_CH1CV(timerx) 	=					  cnt_tar;				
        TIMER_INTF(timerx) 		= (uint16_t)~TIMER_INTF_CH1IF;	
		TIMER_DMAINTEN(timerx) |=			 TIMER_INTF_CH1IF;			
    }
    else if (_CC == 3)
    {
        s_TIM4_CallBack3 = (void (*)(void))_pCallBack;

		TIMER_CH2CV(timerx)  	= 					  cnt_tar;	
        TIMER_INTF(timerx)   	= (uint16_t)~TIMER_INTF_CH2IF;	
		TIMER_DMAINTEN(timerx) |= 			 TIMER_INTF_CH2IF;			
    }
    else if (_CC == 4)
    {
        s_TIM4_CallBack4 = (void (*)(void))_pCallBack;

		TIMER_CH3CV(timerx) 	= 					  cnt_tar;	
        TIMER_INTF(timerx)		= (uint16_t)~TIMER_INTF_CH3IF;	
		TIMER_DMAINTEN(timerx) |= 			 TIMER_INTF_CH3IF;	
    }
	else
    {
        return;
    }
#elif STM32
#if STM32 && BSP_SINGLE_HW_TIMER
    bsp_tim2_start_cc(_CC, _uiTimeOut, _pCallBack);
#else
	TIM_TypeDef* TIMx = TIM_STROBE_5_8;
    
    cnt_now = TIMx->CNT; 
    cnt_tar = cnt_now + _uiTimeOut;			
    if (_CC == 1)
    {
        s_TIM4_CallBack1 = (void (*)(void))_pCallBack;

		TIMx->CCR1 = cnt_tar; 			    
        TIMx->SR = (uint16_t)~TIM_IT_CC1;  
		TIMx->DIER |= TIM_IT_CC1;			
	}
    else if (_CC == 2)
    {
		s_TIM4_CallBack2 = (void (*)(void))_pCallBack;

		TIMx->CCR2 = cnt_tar;				
        TIMx->SR = (uint16_t)~TIM_IT_CC2;	
		TIMx->DIER |= TIM_IT_CC2;			
	}
    else if (_CC == 3)
    {
        s_TIM4_CallBack3 = (void (*)(void))_pCallBack;

		TIMx->CCR3 = cnt_tar;				
        TIMx->SR = (uint16_t)~TIM_IT_CC3;	
		TIMx->DIER |= TIM_IT_CC3;			
	}
    else if (_CC == 4)
    {
        s_TIM4_CallBack4 = (void (*)(void))_pCallBack;

		TIMx->CCR4 = cnt_tar;				
        TIMx->SR = (uint16_t)~TIM_IT_CC4;	
		TIMx->DIER |= TIM_IT_CC4;			
	}
	else
    {
        return;
    }
	#endif
#endif
}


//******************************** start timer  ********************************//

//******************************** timer interupt callback ********************************//
#if GD32
void tim_prog_callback(void)   //TIM1
#elif STM32
void TIM2_IRQHandler(void)
#endif
{
#if GD32
	if ((TIMER_INTF(PROG_TRIG_TIMER) & TIMER_INTF_CH0IF) && (TIMER_DMAINTEN(PROG_TRIG_TIMER) & TIMER_DMAINTEN_CH0IE))
    {
        TIMER_INTF(PROG_TRIG_TIMER) 	  	= ~TIMER_INTF_CH0IF;
        TIMER_DMAINTEN(PROG_TRIG_TIMER) &= ~TIMER_DMAINTEN_CH0IE;
        s_TIM1_CallBack1();
    }
	if ((TIMER_INTF(PROG_TRIG_TIMER) & TIMER_INTF_CH1IF) && (TIMER_DMAINTEN(PROG_TRIG_TIMER) & TIMER_DMAINTEN_CH1IE))
    {
        TIMER_INTF(PROG_TRIG_TIMER) 	  	= ~TIMER_INTF_CH1IF;
        TIMER_DMAINTEN(PROG_TRIG_TIMER) &= ~TIMER_DMAINTEN_CH1IE;
        s_TIM1_CallBack2();
    }
    if ((TIMER_INTF(PROG_TRIG_TIMER) & TIMER_INTF_CH2IF) && (TIMER_DMAINTEN(PROG_TRIG_TIMER) & TIMER_DMAINTEN_CH2IE))
    {
        TIMER_INTF(PROG_TRIG_TIMER) 	  	= ~TIMER_INTF_CH2IF;
        TIMER_DMAINTEN(PROG_TRIG_TIMER) &= ~TIMER_DMAINTEN_CH2IE;
        s_TIM1_CallBack3();
    }
    if ((TIMER_INTF(PROG_TRIG_TIMER) & TIMER_INTF_CH3IF) && (TIMER_DMAINTEN(PROG_TRIG_TIMER) & TIMER_DMAINTEN_CH3IE))
    {
        TIMER_INTF(PROG_TRIG_TIMER)      = ~TIMER_INTF_CH3IF;
        TIMER_DMAINTEN(PROG_TRIG_TIMER) &= ~TIMER_DMAINTEN_CH3IE;
        s_TIM1_CallBack4();
    }
#elif STM32
	uint16_t itstatus = 0x0, itenable = 0x0;
	TIM_TypeDef* TIMx = PROG_TRIG_TIMER;
	
  	itstatus = TIMx->SR & TIM_IT_CC1;
	itenable = TIMx->DIER & TIM_IT_CC1;
    
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC1;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC1;		/* 禁能CC1中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM2_CallBack1();
    }

	itstatus = TIMx->SR & TIM_IT_CC2;
	itenable = TIMx->DIER & TIM_IT_CC2;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC2;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC2;		/* 禁能CC2中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM2_CallBack2();
    }

	itstatus = TIMx->SR & TIM_IT_CC3;
	itenable = TIMx->DIER & TIM_IT_CC3;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC3;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC3;		/* 禁能CC2中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM2_CallBack3();
    }

	itstatus = TIMx->SR & TIM_IT_CC4;
	itenable = TIMx->DIER & TIM_IT_CC4;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC4;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC4;		/* 禁能CC4中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM2_CallBack4();
    }
#endif	
}

void TIM4_IRQHandler(void)
{
	uint16_t itstatus = 0x0, itenable = 0x0;
	TIM_TypeDef* TIMx = TIM4;
	
  	itstatus = TIMx->SR & TIM_IT_CC1;
	itenable = TIMx->DIER & TIM_IT_CC1;
    
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC1;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC1;		/* 禁能CC1中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM4_CallBack1();
    }

	itstatus = TIMx->SR & TIM_IT_CC2;
	itenable = TIMx->DIER & TIM_IT_CC2;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC2;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC2;		/* 禁能CC2中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM4_CallBack2();
    }

	itstatus = TIMx->SR & TIM_IT_CC3;
	itenable = TIMx->DIER & TIM_IT_CC3;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC3;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC3;		/* 禁能CC2中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM4_CallBack3();
    }

	itstatus = TIMx->SR & TIM_IT_CC4;
	itenable = TIMx->DIER & TIM_IT_CC4;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC4;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC4;		/* 禁能CC4中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM4_CallBack4();
    }
}

void TIM1_CC_IRQHandler(void)
{
	uint16_t itstatus = 0x0, itenable = 0x0;
	TIM_TypeDef* TIMx = TIM1;
	
  	itstatus = TIMx->SR & TIM_IT_CC1;
	itenable = TIMx->DIER & TIM_IT_CC1;
    
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC1;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC1;		/* 禁能CC1中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM1_CallBack1();
    }

	itstatus = TIMx->SR & TIM_IT_CC2;
	itenable = TIMx->DIER & TIM_IT_CC2;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC2;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC2;		/* 禁能CC2中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM1_CallBack2();
    }

	itstatus = TIMx->SR & TIM_IT_CC3;
	itenable = TIMx->DIER & TIM_IT_CC3;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC3;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC3;		/* 禁能CC2中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM1_CallBack3();
    }

	itstatus = TIMx->SR & TIM_IT_CC4;
	itenable = TIMx->DIER & TIM_IT_CC4;
	if ((itstatus != (uint16_t)RESET) && (itenable != (uint16_t)RESET))
	{
		TIMx->SR 	= (uint16_t)~TIM_IT_CC4;
		TIMx->DIER &= (uint16_t)~TIM_IT_CC4;		/* 禁能CC4中断 */	

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM1_CallBack4();
    }
}



//******************************** timer interupt callback ********************************//





