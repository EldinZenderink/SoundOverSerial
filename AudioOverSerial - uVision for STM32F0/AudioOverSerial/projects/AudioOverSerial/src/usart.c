/******************************************************************************
 * File           : USART driver implementation file
 *****************************************************************************/
#include "stm32f0xx.h"
#include "usart.h"
#include <stdlib.h>
// ----------------------------------------------------------------------------
// Global variables
// ----------------------------------------------------------------------------
volatile char rx_buffer;

//buffer for the characters
volatile char *fifo_char_buffer;

//stores the user defined size of the buffer
volatile int buffersize;

//keeps track of the r/w position of the buffer.
volatile int bufferposition;

//to make sure nothing is written to the buffer when a character is being read 
volatile uint8_t bufferinuse = 0;

// ----------------------------------------------------------------------------
// Local function prototypes
// ----------------------------------------------------------------------------
void USART_BaudrateDetect(void);

void USART_init(int bufsize)
{
	
  //allocate dynamic buffer (might be usefull if you know your exact length of the string you want to receive ;)
	fifo_char_buffer = (char *)malloc(bufsize * sizeof *fifo_char_buffer);
	
	//setting up global buffer variables.
	buffersize = bufsize;
	bufferposition = 0;
	
  // GPIOA Periph clock enable
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
  
  // PA9 and PA10 Alternate function mode
  GPIOA->MODER |= (GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1);
  
  // Set alternate functions AF1 for PA9 and PA10
  GPIOA->AFR[1] |= 0x00000110;
  
  // USART1 clock enable
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN; 

  // 115200 Bd @ 48 MHz
  // USARTDIV = 48 MHz / 960000 = 50
  // BRR[15:4] = USARTDIV[15:4]
  // When OVER8 = 0, BRR [3:0] = USARTDIV [3:0]
  USART1->BRR = (uint32_t) 50;

  // USART enable
  // Receiver enable
  // Transmitter enable
  USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;

  // Default value
  USART1->CR2 = 0;
  USART1->CR3 = 0; 
  
//   // RXNE interrupt enable
  USART1->CR1 |= USART_CR1_RXNEIE;
//   
//   // USART1 interrupts enable in NVIC
  NVIC_EnableIRQ(USART1_IRQn);
  NVIC_SetPriority(USART1_IRQn, 0);
  NVIC_ClearPendingIRQ(USART1_IRQn);
	

}




void USART_putc(char c)
{
  // Wait for Transmit data register empty
  while((USART1->ISR & USART_ISR_TXE) == 0) ;

  // Transmit data by writing to TDR, clears TXE flag  
  USART1->TDR = c;
}

void USART_putstr(char *str)
{
  while(*str)
  {
    if(*str == '\n')
    {
      USART_putc('\r');
    }
    
    USART_putc(*str++);
  }
}

char USART_getc(void)
{
  char c;

  // Was there an Overrun error?
  if((USART1->ISR & USART_ISR_ORE) != 0)
  {
    // Yes, clear it 
    USART1->ICR |= USART_ICR_ORECF;
  }

  // Wait for data in the Receive Data Register
  while((USART1->ISR & USART_ISR_RXNE) == 0) ;

  // Read data from RDR, clears the RXNE flag
  c = (char)USART1->RDR;

  return(c);
}


void USART_getstr(char *str)
{
	
	int i = 0;
  // Implement this function yourself
	// Was there an Overrun error?
  if((USART1->ISR & USART_ISR_ORE) != 0)
  {
    // Yes, clear it 
    USART1->ICR |= USART_ICR_ORECF;
  }
	
	while(1){
		while((USART1->ISR & USART_ISR_RXNE) == 0);
		if( (str[i] = (char)USART1->RDR) == '\r'){
			break;
		}
		
		i++;
	}
	
	
}


//return first item in buffer, if buffer is empty, return \0
// - read first character into character pointer
// - start shifting buffer 
//		- example: current buffer =  {a, b, c, d, e};
//    1. starts reading at index + 1 (if index = 0, reads at index 1-> = b) into char var
//    2. puts value of char var at current index in current buffer (if index  =0 -> new buffer  = {b,b,c,d,e}, index = 1 -> new buffer = {b,c,c,d,e} etc)
//    3. decrease current bufferposition with 1
//    4. start at 1 until end of bufferposition (-1, since we have one less character)
// - return current bufferposition after shifting

int USART_getcb(char *character){
	
	//setup the default variables.
	uint16_t i;
	*character = '\0';
	
	//check if there is data in the buffer (bufferposition increments everytime new characters are added)
	if(bufferposition > 0){
		
		//make sure buffer is not adding characters while shifting / reading
		bufferinuse = 1;
		
		//read the first available character in the fifo buffer
		*character = fifo_char_buffer[0];
		
		//start shifting the characters after reading the first character
		//running up to bufferposition -1, since we are deleting the first character in the buffer
		for(i = 0; i < bufferposition - 1; i++){
			//get the character starting from index i with offset +1 (skipping the first character
			char shiftchar = fifo_char_buffer[i + 1];
			//put the character at the current index (shifting it by one)
			fifo_char_buffer[i] = shiftchar;
		}
		
		//set the buffer position correctly (removed a char, so -1);
		bufferposition = bufferposition - 1;		
		
		//buffer is done shifting, can be writen to again.
		bufferinuse = 0;
	}
	

	return bufferposition;
}

// Implements the following VT100 terminal commands
// - Clear screan
// - Cursor home
void USART_clearscreen(void)
{
  char cmd1[5] = {0x1B, '[', '2', 'J', '\0'}; // Clear screen
  char cmd2[4] = {0x1B, '[', 'f', '\0'}; // Cursor home
  
  USART_putstr(cmd1);
  USART_putstr(cmd2);
}
