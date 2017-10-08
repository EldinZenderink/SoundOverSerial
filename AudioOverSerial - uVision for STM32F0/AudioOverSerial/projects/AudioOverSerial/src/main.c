/******************************************************************************
 * File           : Main program - AudioOverSerial
 *****************************************************************************/
#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "stm32f0xx_dac.h"
#include "usart.h"

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------
void delay(const int d);


// ----------------------------------------------------------------------------
// Global Variables
// ----------------------------------------------------------------------------

volatile int position;
volatile uint16_t compare;
volatile uint8_t selectMode = 0;
volatile uint16_t data;
volatile uint8_t bytesreceived = 0;
volatile char sixteenbitdata[2];

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
int main(void)
{
	
	//initiate necessary peripherals setup stuff
	GPIO_InitTypeDef GPIO_InitStructure;
  DAC_InitTypeDef  DAC_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_OCInitTypeDef       TIM_OCInitStructure;
	
  //(+) Enable DAC APB1 clock to get write access to DAC registers
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	
	//(+) Configure Pin PC7 as PWM out
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_1);
	
	//(+) Settig up the timer timing for the pwm signal
	// using a period/resolution of 1024
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period        = 1024 - 1;
  TIM_TimeBaseStructure.TIM_Prescaler     = (uint16_t)((SystemCoreClock / 48000000) - 1);
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
	//(+) Settig up the timer for the pwm signal with output compare mode
	TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse       = compare;
  TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High; 
	
	//(+) start the timer
  TIM_OC2Init(TIM3, &TIM_OCInitStructure);
  TIM_Cmd(TIM3, ENABLE);
	
	
  //(+) Configure DAC_OUT1 (DAC_OUT1: PA4) in analog mode
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  //(+) Configure the DAC channel using DAC_Init()
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);  
	
  //(+) Enable the DAC channel using DAC_Cmd()
  DAC_Cmd(DAC_Channel_1, ENABLE);
	
	// Initialize LED 3
	STM_EVAL_LEDInit(LED3);
  // Initialize User Button on STM32F0-Discovery
  STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	
	//Initialize USART connection
	USART_init(100);
  USART_clearscreen();
	STM_EVAL_LEDOn(LED3);
	//Selecting mode (PWM or DAC), default = DAC
  while(1)
  {
		if (STM_EVAL_PBGetState(BUTTON_USER)== SET) {
			if(selectMode){
				selectMode = 0;
				STM_EVAL_LEDOn(LED3);
			} else {
				selectMode = 1;				
			  STM_EVAL_LEDOff(LED3);
			}
			
			
		}
		delay(500);
  }
}

//USART interrupt for receiving
void USART1_IRQHandler(){
	 if(USART1->ISR & USART_ISR_RXNE)
   {		 
		 //make sure that received value is actual 16 bit uint (reset sometimes necesary if usart connection is reinitialized (bytesreceived could have offset 1, which results in alot of noise)
		 if(bytesreceived > 1){
			 
			 //create 16bit uint
			 data = (sixteenbitdata[1] << 8) | sixteenbitdata[0];
			 
			 //PWM or DAC
			 if(selectMode){
				  TIM_SetCompare2(TIM3, (data));
			 } else {				 
					DAC_SetChannel1Data(DAC_Align_12b_R, data);
			 }

			 bytesreceived = 0;
		 }
		 
     //putting data in array for conversion to 16 uint
		 sixteenbitdata[bytesreceived] = USART1->RDR;
		 
		 bytesreceived++;
		 
		 //clear intterupt flag
		 USART1->ICR |= USART_ICR_ORECF;
		 
   }
}


void delay(const int d)
{
  volatile int i;
  for (i=d; i>0; i--);
  return;
}
