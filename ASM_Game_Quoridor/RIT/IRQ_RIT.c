/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_RIT.c
** Last modified Date:  2014-09-25
** Last Version:        V1.00
** Descriptions:        functions to manage T0 and T1 interrupts
** Correlated files:    RIT.h
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
#include "lpc17xx.h"
#include "RIT.h"
#include "../game/game.h"

/******************************************************************************
** Function name:		RIT_IRQHandler
**
** Descriptions:		REPETITIVE INTERRUPT TIMER handler
**
** parameters:			None
** Returned value:		None
**
******************************************************************************/

extern int menu;
extern int turnoCorrente;
extern int id;
extern int multiplayer;

void RIT_IRQHandler (void)
{					
	static int J_select=0;
	static int J_down = 0;
	static int J_left = 0;	
	static int J_right = 0;
	static int J_up = 0;
		
	//select
  if((LPC_GPIO1->FIOPIN & (1<<25)) == 0)
	{	
		J_select++;
		switch(J_select){
			case 1:
				//select
				if(menu > 0)
				{
					menuGestisci(0);
				}
				else
				{
					if(multiplayer == 1 && turnoCorrente != id)
					{
						
					}
					else
					{
						move(0);
					}
				}
				break;
			default:
				break;
		}
	}
	else{
		J_select=0;
	}
	
	//down
	if((LPC_GPIO1->FIOPIN & (1<<26)) == 0)
	{	
		J_down++;
		switch(J_down){
			case 1:
				//move down
				if(menu > 0)
				{
					menuGestisci(1);
				}
				else
				{
					if(multiplayer == 1 && turnoCorrente != id)
					{
						
					}
					else
					{
						move(1);
					}
				}
				break;
			default:
				break;
		}
	}
	else{
		J_down=0;
	}
	
	//left
	if((LPC_GPIO1->FIOPIN & (1<<27)) == 0)
	{	
		J_left++;
		switch(J_left){
			case 1:
				//move left
				if(menu > 0)
				{
					
				}
				else
				{
					if(multiplayer == 1 && turnoCorrente != id)
					{
						
					}
					else
					{
						move(2);
					}
				}
				break;
			default:
				break;
		}
	}
	else{
		J_left=0;
	}
	
	//right
	if((LPC_GPIO1->FIOPIN & (1<<28)) == 0)
	{	
		J_right++;
		switch(J_right){
			case 1:
				//move right
				if(menu > 0)
				{
					
				}
				else
				{
					if(multiplayer == 1 && turnoCorrente != id)
					{
						
					}
					else
					{
						move(3);
					}
				}
				break;
			default:
				break;
		}
	}
	else{
		J_right=0;
	}
	
	//up
	if((LPC_GPIO1->FIOPIN & (1<<29)) == 0)
	{	
		J_up++;
		switch(J_up){
			case 1:
				//move up
				if(menu > 0)
				{
					menuGestisci(2);
				}
				else
				{
					if(multiplayer == 1 && turnoCorrente != id)
					{
						
					}
					else
					{
						move(4);
					}
				}
				break;
			default:
				break;
		}
	}
	else{
		J_up=0;
	}
	
  LPC_RIT->RICTRL |= 0x1;	/* clear interrupt flag */
  return;
}

/******************************************************************************
**                            End Of File
******************************************************************************/
