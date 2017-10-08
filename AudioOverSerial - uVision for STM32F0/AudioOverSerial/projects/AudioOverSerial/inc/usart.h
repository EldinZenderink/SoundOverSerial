/******************************************************************************
 * Project        : HAN ESE PRJ2, PRJ1V & PRJ1D
 * File           : USART driver header file
 * Copyright      : 2013 HAN Embedded Systems Engineering
 ******************************************************************************
  Change History:

    Version 1.0 - April 2013
    > Initial revision

******************************************************************************/
#ifndef _USART_H_
#define _USART_H_

#include "stdint.h"

/******************************************************************************
  Defines
******************************************************************************/


/******************************************************************************
  Function prototypes
******************************************************************************/
void USART_init(int bufsize);
void USART_putc(char c);
char USART_getc(void);
void USART_putstr(char *str);
void USART_getstr(char *str);
void USART_clearscreen(void);
int USART_getcb(char *character); //new fifo buffer get character method
#endif // _USART_H_
