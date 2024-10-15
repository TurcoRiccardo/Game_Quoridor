/****************************************Copyright (c)****************************************************
**                                      
**                                 http://www.powermcu.com
**
**--------------File Info---------------------------------------------------------------------------------
** File name:               main.c
** Descriptions:            The GLCD application function
**
**--------------------------------------------------------------------------------------------------------
** Created by:              AVRman
** Created date:            2010-11-7
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             Paolo Bernardi
** Modified date:           03/01/2020
** Version:                 v2.0
** Descriptions:            basic program for LCD and Touch Panel teaching
**
*********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "LPC17xx.h"
#include "GLCD/GLCD.h" 
#include "timer/timer.h"
#include "joystick/joystick.h"
#include "button_EXINT/button.h"
#include "RIT/RIT.h"
#include "CAN/CAN.h"
#include "game/game.h"

#define SIMULATOR 1

#ifdef SIMULATOR
extern uint8_t ScaleFlag; // <- ScaleFlag needs to visible in order for the emulator to find the symbol (can be placed also inside system_LPC17xx.h but since it is RO, it needs more work)
#endif

extern int menu;
extern int gameStart;
extern int multiplayer;
extern int id;

int main(void)
{
  SystemInit();  												/* System Initialization (i.e., PLL)  */
	BUTTON_init();
	joystick_init();
  LCD_Initialization(); 								//dimensioni schermo 320*240
	
	init_timer(0, 0x017D7840);						//timer0 1sec
	
	init_RIT(0x004C4B40);									/* RIT Initialization 100 msec       	*/
	enable_RIT();													/* RIT enabled												*/
	
	CAN_Init();														//Inizializzo can
	
	LPC_SC->PCON |= 0x1;									/* power-down	mode										*/
	LPC_SC->PCON &= ~(0x2);						
	
	//blocco i bottoni
	NVIC_DisableIRQ(EINT1_IRQn);
	NVIC_DisableIRQ(EINT2_IRQn);
	
	menu = 1;
	gameStart = 0;
	id = 1;
	
  while (1)	
  {
		__ASM("wfi");
		if(gameStart == 1)
		{
			game_setup();
			turno();
			gameStart=0;
		}
  }
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
