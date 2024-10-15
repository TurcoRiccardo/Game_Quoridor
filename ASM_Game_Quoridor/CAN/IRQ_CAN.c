/*----------------------------------------------------------------------------
 * Name:    Can.c
 * Purpose: CAN interface for for LPC17xx with MCB1700
 * Note(s): see also http://www.port.de/engl/canprod/sv_req_form.html
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2009 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <lpc17xx.h>                  /* LPC17xx definitions */
#include "CAN.h"                      /* LPC17xx CAN adaption layer */
#include "../game/game.h"

extern uint8_t icr ; 										//icr and result must be global in order to work with both real and simulated landtiger.
extern uint32_t result;
extern CAN_msg       CAN_TxMsg;    /* CAN message for sending */
extern CAN_msg       CAN_RxMsg;    /* CAN message for receiving */                                
extern int menu;
extern int multiplayer;
extern int npc;
extern int gameStart;
extern int modalita;
extern int cordinataX;
extern int cordinataY;
extern int muritempX;
extern int muritempY;
extern int ruota;		
volatile int id;

/*----------------------------------------------------------------------------
  CAN interrupt handler
 *----------------------------------------------------------------------------*/
void CAN_IRQHandler (void)
{
  int idm = 0;
	
	/* check CAN controller 1 */
	icr = 0;
  icr = (LPC_CAN1->ICR | icr) & 0xFF;               /* clear interrupts */
	
  if (icr & (1 << 0)) {                          		/* CAN Controller #1 meassage is received */
		CAN_rdMsg (1, &CAN_RxMsg);	                		/* Read the message */
    LPC_CAN1->CMR = (1 << 2);                    		/* Release receive buffer */
		
		if(menu > 0)
		{
			multiplayer = 1;
			npc = 0;
			if(CAN_RxMsg.data[0] == 1)
			{
				id = 2;
			}
			else
			{
				id = 1;
			}
			gameStart = 1;
		}
		if(menu == 0 && multiplayer == 1)
		{
			idm = CAN_RxMsg.data[0];
			modalita = CAN_RxMsg.data[1] >> 4;
			ruota = CAN_RxMsg.data[1] & 0xF0;
			if(modalita == 0)
			{
				cordinataX = CAN_RxMsg.data[2];
				cordinataY = CAN_RxMsg.data[3];
			}
			else
			{
				muritempX = CAN_RxMsg.data[2];
				muritempY = CAN_RxMsg.data[3];
			}
			
			if((modalita == 0) && ((idm == 1 && cordinataY == 6) || (idm == 2 && cordinataY == 0)))
			{
				lose();
			}
			else
			{
				cambioTurno();
			}
		}
		
  }
	else
	{
		if (icr & (1 << 1)) 													/* CAN Controller #1 meassage is transmitted */
		{                        
			
		}
	}
	
	
	/* check CAN controller 2 */
	icr = 0;
	icr = (LPC_CAN2->ICR | icr) & 0xFF;             /* clear interrupts */
	if (icr & (1 << 0)) {                          	/* CAN Controller #2 meassage is received */
		CAN_rdMsg (2, &CAN_RxMsg);	                	/* Read the message */
    LPC_CAN2->CMR = (1 << 2);                    	/* Release receive buffer */
		
	}
	else
	{
		if (icr & (1 << 1))                           /* CAN Controller #2 meassage is transmitted */
		{                         
			
		}
	}
}
