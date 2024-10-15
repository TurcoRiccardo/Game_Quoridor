#include "button.h"
#include "lpc17xx.h"
#include "../game/game.h"

extern int menu;

void EINT0_IRQHandler (void)	  	/* INT0														 */
{		
	//game start
	if(menu == 1)
	{
		menu_init();
	}
	LPC_SC->EXTINT &= (1 << 0);     /* clear pending interrupt         */
}


void EINT1_IRQHandler (void)	  	/* KEY1														 */
{
	//Cambio modalita
	cambioModalita();
	
	LPC_SC->EXTINT &= (1 << 1);     /* clear pending interrupt         */
}


void EINT2_IRQHandler (void)	  	/* KEY2														 */
{
	//ruoto muro
	ruotaMuro();
	
  LPC_SC->EXTINT &= (1 << 2);     /* clear pending interrupt         */    
}


