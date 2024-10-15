#include "game.h"
#include "lpc17xx.h"
#include "../timer/timer.h"
#include "../GLCD/GLCD.h"
#include "../RIT/RIT.h"
#include "../CAN/CAN.h"
#include <string.h>
#include <stdlib.h>

volatile int menu;						//se siamo nel menu > 0
volatile int valueTimer;			//Valore del contatore del timer
volatile int multiplayer;			//stiamo giocando singleplayer o multiplayer
volatile int npc;							//sta giocando human o npc
volatile int gameStart;				//indica quando inizia il gioco
volatile int turnoCorrente;		//TurnoGiocatore1 = 1, TurnoGiocatore2 = 2
extern int id;								//id altro giocatore
volatile int modalita;				//ModalitaMovimento = 0, ModalitaMuro = 1
volatile int cordinataX;			//Coordinata X giocatore del turno corrente
volatile int cordinataY;			//Coordinata Y giocatore del turno corrente
int muriP1;										//Numero muri da posizionare Giocatore 1
int muriP2;										//Numero muri da posizionare Giocatore 2
uint32_t mosse[100];					//Lista di mosse dall'inizio della partita
int nmosse;										//Numero della mossa corrente
int matrice[7][7];						//Matrice che tiene traccia delle caselle
int sol[7][7];								//Matrice per il controllo anti "trap"
int muri[6][6];								//Matrice che tiene traccia dei muri piazzati
volatile int muritempX;				//Coordinata X del muro da piazzare
volatile int muritempY;				//Coordinata Y del muro da piazzare
volatile int ruota;						//Orizzontale = 1, Verticale = 2

void game_setup()
{
	int i, j;
	
	//schermo
	LCD_Clear(Blue2); 
	LCD_DrawTable(14, 0, Black);
	LCD_DrawLine(0, 220, 240, 220, Black);
	LCD_DrawLine(0, 221, 240, 221, Black);
	LCD_DrawLine(0, 222, 240, 222, Black);
	LCD_DrawRectangle(0, 240, Black);
	LCD_DrawRectangle(80, 240, Black);
	LCD_DrawRectangle(160, 240, Black);
	GUI_Text(10, 245, (uint8_t *) "P1 Wall", Black, Blue2);
	GUI_Text(35, 270, (uint8_t *) "8", Black, Blue2);
	GUI_Text(105, 255, (uint8_t *) "20 s", Black, Blue2);
	GUI_Text(170, 245, (uint8_t *) "P2 Wall", Black, Blue2);
	GUI_Text(195, 270, (uint8_t *) "8", Black, Blue2);
	
	//resetto matrici
	for(i=0; i<7; i++)
	{
		for(j=0; j<7; j++)
		{
			matrice[i][j] = 0;
		}
	}
	for(i=0; i<6; i++)
	{
		for(j=0; j<6; j++)
		{
			muri[i][j] = 0;
		}
	}
	for(i=0; i<100; i++)
	{
		mosse[i] = 0;
	}
	
	//player1
	LCD_DrawPlayer(104, 0, White);
	matrice[3][0] = 1;
	muriP1=8;
	//player2
	LCD_DrawPlayer(104, 180, Red);
	matrice[3][6] = 2;
	muriP2=8;
	
	//variabili
	turnoCorrente = 1;
	nmosse = 0;
	menu = 0;
	modalita = 0;
	ruota = 1;
	
	//can
	if(multiplayer == 1 && id == 1)
	{
		CAN_TxMsg.data[0] = (turnoCorrente & 0xFF);
		CAN_TxMsg.len = 1;
		CAN_TxMsg.id = 1;
		CAN_TxMsg.format = STANDARD_FORMAT;
		CAN_TxMsg.type = DATA_FRAME;
		CAN_wrMsg (1, &CAN_TxMsg);
	}
	
	//abilito bottone
	NVIC_ClearPendingIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
	LPC_SC->EXTINT &= (1 << 1);
	
}

void turno()
{
	char s[30]="";
	valueTimer = 20;
	enable_timer(0);
	
	//stampo turno
	sprintf(s, "    Turno di P%d    ", turnoCorrente);
	GUI_Text(40, 300, (uint8_t *) s, Black, Blue2);
	
	//stampo timer
	GUI_Text(105, 255, (uint8_t *) "20 s", Black, Blue2);
	
	//modalita movimento
	modalita = 0;
	ruota = 1;
	muritempX=-1;
	muritempY=-1;
	
	
	gioco();
}

void cambioModalita()
{
	if(modalita == 0)
	{
		modalita = 1;
		
		NVIC_ClearPendingIRQ(EINT2_IRQn);
		NVIC_EnableIRQ(EINT2_IRQn);
		LPC_SC->EXTINT &= (1 << 2);
	}
	else
	{
		modalita = 0;
		
		NVIC_DisableIRQ(EINT2_IRQn);
	}
	gioco();
}

void ruotaMuro()
{
	//cancello muro precedente
	LCD_DrawWall(muritempX*30+14, muritempY*30, Blue2, ruota);
	
	//rifaccio i muri di contorno
	if(muritempY < 5)
	{
		//down
		if(muri[muritempX][muritempY+1] != 0)
		{
			LCD_DrawWall(muritempX*30+14, (muritempY+1)*30, Green, muri[muritempX][muritempY+1]);
		}
	}
	if(muritempX > 0)
	{
		//left
		if(muri[muritempX-1][muritempY] != 0)
		{
			LCD_DrawWall((muritempX-1)*30+14, muritempY*30, Green, muri[muritempX-1][muritempY]);
		}
	}
	if(muritempX < 5)
	{
		//right
		if(muri[muritempX+1][muritempY] != 0)
		{
			LCD_DrawWall((muritempX+1)*30+14, muritempY*30, Green, muri[muritempX+1][muritempY]);
		}
	}
	if(muritempY > 0)
	{
		//up
		if(muri[muritempX][muritempY-1] != 0)
		{
			LCD_DrawWall(muritempX*30+14, (muritempY-1)*30, Green, muri[muritempX][muritempY-1]);
		}
	}
	if(muri[muritempX][muritempY] != 0)
	{
		//corrente
		LCD_DrawWall(muritempX*30+14, muritempY*30, Green, muri[muritempX][muritempY]);
	}
	
	//ruoto e stampo muro 
	if(ruota == 1)
	{
		ruota = 2;
	}
	else
	{
		ruota = 1;
	}
	LCD_DrawWall(muritempX*30+14, muritempY*30, Cyan, ruota);
}

void cambioTurno()
{
	int i, j;
	char s[5]="";
	
	//can
	if(multiplayer == 1 && turnoCorrente == id)
	{
		//invio il mio turno
		CAN_TxMsg.data[0] = (turnoCorrente & 0xFF);
		CAN_TxMsg.data[1] = ((modalita & 0xF0) << 4) + (ruota & 0x0F);
		if(modalita == 0)
		{
			CAN_TxMsg.data[2] = cordinataX & 0xFF;
			CAN_TxMsg.data[3] = cordinataY & 0xFF;
		}
		else
		{
			CAN_TxMsg.data[2] = muritempX & 0xFF;
			CAN_TxMsg.data[3] = muritempY & 0xFF;
		}
		CAN_TxMsg.len = 4;
		CAN_TxMsg.id = turnoCorrente;
		CAN_TxMsg.format = STANDARD_FORMAT;
		CAN_TxMsg.type = DATA_FRAME;
		CAN_wrMsg (1, &CAN_TxMsg);
	}
	else if (multiplayer == 1)
	{
		//ricevo il turno
		if (modalita == 0)
		{
			for(i=0; i<7; i++)
			{
				for(j=0; j<7; j++)
				{
					if(matrice[i][j] == turnoCorrente)
					{
						LCD_DrawPlayer(i*30+14, j*30, Blue2);
						matrice[i][j] = 0;
					}
				}
			}
			if (turnoCorrente == 1)
			{
				LCD_DrawPlayer(cordinataX*30+14, cordinataY*30, White);
				matrice[cordinataX][cordinataY] = turnoCorrente;
			}
			else
			{
				LCD_DrawPlayer(cordinataX*30+14, cordinataY*30, Red);
				matrice[cordinataX][cordinataY] = turnoCorrente;
			}
		}
		else
		{
			if(turnoCorrente == 1)
			{
				LCD_DrawWall(muritempX*30+14, muritempY*30, Green, ruota);
				muriP1--;
				sprintf(s, "%d", muriP1);
				GUI_Text(35, 270, (uint8_t *) s, Black, Blue2);
			}
			else
			{
				LCD_DrawWall(muritempX*30+14, muritempY*30, Green, ruota);
				muriP2--;
				sprintf(s, "%d", muriP2);
				GUI_Text(195, 270, (uint8_t *) s, Black, Blue2);
			}
		}
	}
	
	//salvo in mosse
	mosse[nmosse] = (turnoCorrente - 1) + (modalita*2);
	if(modalita == 1)
	{
		if(ruota == 1)
		{
			mosse[nmosse] += (1*4);
		}
		mosse[nmosse] += (muritempX*8) + (muritempY*2048);
	}
	else
	{
		mosse[nmosse] += (cordinataX*8) + (cordinataY*2048);
	}
	nmosse++;
	//cambio turno
	if(turnoCorrente == 1)
	{
		turnoCorrente = 2;
	}
	else
	{
		turnoCorrente = 1;
	}
	reset_timer(0);
	disable_timer(0);
	turno();
}


void gioco()
{
	int i, j;
	
	//cancello barra se presente
	if(muritempX != -1)
	{
		//cancello muro precedente
		LCD_DrawWall(muritempX*30+14, muritempY*30, Blue2, ruota);
		
		//rifaccio i muri di contorno
		if(muritempY < 5)
		{
			//down
			if(muri[muritempX][muritempY+1] != 0)
			{
				LCD_DrawWall(muritempX*30+14, (muritempY+1)*30, Green, muri[muritempX][muritempY+1]);
			}
		}
		if(muritempX > 0)
		{
			//left
			if(muri[muritempX-1][muritempY] != 0)
			{
				LCD_DrawWall((muritempX-1)*30+14, muritempY*30, Green, muri[muritempX-1][muritempY]);
			}
		}
		if(muritempX < 5)
		{
			//right
			if(muri[muritempX+1][muritempY] != 0)
			{
				LCD_DrawWall((muritempX+1)*30+14, muritempY*30, Green, muri[muritempX+1][muritempY]);
			}
		}
		if(muritempY > 0)
		{
			//up
			if(muri[muritempX][muritempY-1] != 0)
			{
				LCD_DrawWall(muritempX*30+14, (muritempY-1)*30, Green, muri[muritempX][muritempY-1]);
			}
		}
		if(muri[muritempX][muritempY] != 0)
		{
			//corrente
			LCD_DrawWall(muritempX*30+14, muritempY*30, Green, muri[muritempX][muritempY]);
		}
	}
	
	//prendo coordinate
	for(i=0; i<7; i++)
	{
		for(j=0; j<7; j++)
		{
			if(matrice[i][j] == turnoCorrente)
			{
				cordinataX = i;
				cordinataY = j;
			}
			if(matrice[i][j] == 3)
			{
				LCD_DrawPlayer(i*30+14, j*30, Blue2);
				matrice[i][j] = 0;
			}
		}
	}
	if(multiplayer == 1 && turnoCorrente != id)
	{
		//non faccio nulla
	}
	else if(npc == 1 && turnoCorrente == 2)
	{
		NPCturno();
	}
	else
	{
		if(modalita == 0)
		{
			//guardo sopra
			if(cordinataY>0)
			{
				if(cordinataX > 0)
				{
					if(cordinataX < 5)
					{
						if(!(muri[cordinataX-1][cordinataY-1] == 1 || muri[cordinataX][cordinataY-1] == 1))
						{
							if (matrice[cordinataX][cordinataY-1] == 0)
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY-1)*30, Yellow);
								matrice[cordinataX][cordinataY-1] = 3;
							}
							else if(cordinataY-1 > 0)
							{
								if(!(muri[cordinataX-1][cordinataY-2] == 1 || muri[cordinataX][cordinataY-2] == 1))
								{
									LCD_DrawPlayer(cordinataX*30+14, (cordinataY-2)*30, Yellow);
									matrice[cordinataX][cordinataY-2] = 3;
								}
							}
						}
					}
					else
					{
						if(muri[cordinataX-1][cordinataY-1] != 1)
						{
							if (matrice[cordinataX][cordinataY-1] == 0)
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY-1)*30, Yellow);
								matrice[cordinataX][cordinataY-1] = 3;
							}
							else if(cordinataY-1 > 0)
							{
								if(muri[cordinataX-1][cordinataY-2] != 1)
								{
									LCD_DrawPlayer(cordinataX*30+14, (cordinataY-2)*30, Yellow);
									matrice[cordinataX][cordinataY-2] = 3;
								}
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX][cordinataY-1] != 1)
					{
						if (matrice[cordinataX][cordinataY-1] == 0)
						{
							LCD_DrawPlayer(cordinataX*30+14, (cordinataY-1)*30, Yellow);
							matrice[cordinataX][cordinataY-1] = 3;
						}
						else if(cordinataY-1 > 0)
						{
							if(muri[cordinataX][cordinataY-2] != 1)
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY-2)*30, Yellow);
								matrice[cordinataX][cordinataY-2] = 3;
							}
						}
					}
				}
			}
			//guardo sotto
			if(cordinataY<6)
			{
				if(cordinataX > 0)
				{
					if(cordinataX < 5)
					{
						if(!(muri[cordinataX-1][cordinataY] == 1 || muri[cordinataX][cordinataY] == 1))
						{
							if (matrice[cordinataX][cordinataY+1] == 0)
							{
								LCD_DrawPlayer(cordinataX*30+14, cordinataY*30+30, Yellow);
								matrice[cordinataX][cordinataY+1] = 3;
							}
							else if(cordinataY+1 < 6)
							{
								if(!(muri[cordinataX-1][cordinataY+1] == 1 || muri[cordinataX][cordinataY+1] == 1))
								{
									LCD_DrawPlayer(cordinataX*30+14, (cordinataY+2)*30, Yellow);
									matrice[cordinataX][cordinataY+2] = 3;
								}
							}
						}
					}
					else
					{
						if(muri[cordinataX-1][cordinataY] != 1)
						{
							if (matrice[cordinataX][cordinataY+1] == 0)
							{
								LCD_DrawPlayer(cordinataX*30+14, cordinataY*30+30, Yellow);
								matrice[cordinataX][cordinataY+1] = 3;
							}
							else if(cordinataY+1 < 6)
							{
								if(muri[cordinataX-1][cordinataY+1] != 1)
								{
									LCD_DrawPlayer(cordinataX*30+14, (cordinataY+2)*30, Yellow);
									matrice[cordinataX][cordinataY+2] = 3;
								}
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX][cordinataY] != 1)
					{
						if (matrice[cordinataX][cordinataY+1] == 0)
						{
							LCD_DrawPlayer(cordinataX*30+14, cordinataY*30+30, Yellow);
							matrice[cordinataX][cordinataY+1] = 3;
						}
						else if(cordinataY+1 < 6)
						{
							if(muri[cordinataX][cordinataY+1] != 1)
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY+2)*30, Yellow);
								matrice[cordinataX][cordinataY+2] = 3;
							}
						}
					}
				}
			}
			//guardo sinistra
			if(cordinataX>0)
			{
				if(cordinataY > 0)
				{
					if(cordinataY < 5)
					{
						if(!(muri[cordinataX-1][cordinataY-1] == 2 || muri[cordinataX-1][cordinataY] == 2))
						{
							if (matrice[cordinataX-1][cordinataY] == 0)
							{
								LCD_DrawPlayer((cordinataX-1)*30+14, cordinataY*30, Yellow);
								matrice[cordinataX-1][cordinataY] = 3;
							}
							else if(cordinataX-1 > 0)
							{
								if(!(muri[cordinataX-2][cordinataY-1] == 2 || muri[cordinataX-2][cordinataY] == 2))
								{
									LCD_DrawPlayer((cordinataX-2)*30+14, cordinataY*30, Yellow);
									matrice[cordinataX-2][cordinataY] = 3;
								}
							}
						}
					}
					else
					{
						if(muri[cordinataX-1][cordinataY-1] != 2)
						{
							if (matrice[cordinataX-1][cordinataY] == 0)
							{
								LCD_DrawPlayer((cordinataX-1)*30+14, cordinataY*30, Yellow);
								matrice[cordinataX-1][cordinataY] = 3;
							}
							else if(cordinataX-1 > 0)
							{
								if(muri[cordinataX-2][cordinataY-1] != 2)
								{
									LCD_DrawPlayer((cordinataX-2)*30+14, cordinataY*30, Yellow);
									matrice[cordinataX-2][cordinataY] = 3;
								}
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX-1][cordinataY] != 2)
					{
						if (matrice[cordinataX-1][cordinataY] == 0)
						{
							LCD_DrawPlayer((cordinataX-1)*30+14, cordinataY*30, Yellow);
							matrice[cordinataX-1][cordinataY] = 3;
						}
						else if(cordinataX-1 > 0)
						{
							if(muri[cordinataX-2][cordinataY] != 2)
							{
								LCD_DrawPlayer((cordinataX-2)*30+14, cordinataY*30, Yellow);
								matrice[cordinataX-2][cordinataY] = 3;
							}
						}
					}
				}
			}
			//guardo destra
			if(cordinataX<6)
			{
				if(cordinataY > 0)
				{
					if(cordinataY < 5)
					{
						if(!(muri[cordinataX][cordinataY-1] == 2 || muri[cordinataX][cordinataY] == 2))
						{
							if (matrice[cordinataX+1][cordinataY] == 0)
							{
								LCD_DrawPlayer((cordinataX+1)*30+14, cordinataY*30, Yellow);
								matrice[cordinataX+1][cordinataY] = 3;
							}
							else if(cordinataX+1 < 6)
							{
								if(!(muri[cordinataX+1][cordinataY-1] == 2 || muri[cordinataX+1][cordinataY] == 2))
								{
									LCD_DrawPlayer((cordinataX+2)*30+14, cordinataY*30, Yellow);
									matrice[cordinataX+2][cordinataY] = 3;
								}
							}
						}
					}
					else
					{
						if(muri[cordinataX][cordinataY-1] != 2)
						{
							if (matrice[cordinataX+1][cordinataY] == 0)
							{
								LCD_DrawPlayer((cordinataX+1)*30+14, cordinataY*30, Yellow);
								matrice[cordinataX+1][cordinataY] = 3;
							}
							else if(cordinataX+1 < 6)
							{
								if(muri[cordinataX+1][cordinataY-1] != 2)
								{
									LCD_DrawPlayer((cordinataX+2)*30+14, cordinataY*30, Yellow);
									matrice[cordinataX+2][cordinataY] = 3;
								}
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX][cordinataY] != 2)
					{
						if (matrice[cordinataX+1][cordinataY] == 0)
						{
							LCD_DrawPlayer((cordinataX+1)*30+14, cordinataY*30, Yellow);
							matrice[cordinataX+1][cordinataY] = 3;
						}
						else if(cordinataX+1 < 6)
						{
							if(muri[cordinataX+1][cordinataY] != 2)
							{
								LCD_DrawPlayer((cordinataX+2)*30+14, cordinataY*30, Yellow);
								matrice[cordinataX+2][cordinataY] = 3;
							}
						}
					}
				}
			}
		}
		else
		{
			//faccio comparire il muro
			muritempX = 3;
			muritempY = 3;
			LCD_DrawWall(muritempX*30+14, muritempY*30, Cyan, ruota);
		}
	}
}


int ric(int cX, int cY, int ris)
{
	if(cY == 6)
	{
		return 1;
	}
	if(ris > 0)
	{
		return ris;
	}
	//giu
	if(cY < 6 && ris == 0 && sol[cX][cY+1] == 0)
	{
		if(cX > 0)
		{
			if(cX < 6)
			{
				if(!(muri[cX-1][cY] == 1 || muri[cX][cY] == 1))
				{
					sol[cX][cY+1] = 1;
					ris += ric(cX, cY+1, ris);
				}
			}
			else
			{
				if(muri[cX-1][cY] != 1)
				{
					sol[cX][cY+1] = 1;
					ris += ric(cX, cY+1, ris);
				}
			}
		}
		else
		{
			if(muri[cX][cY] != 1)
			{
				sol[cX][cY+1] = 1;
				ris += ric(cX, cY+1, ris);
			}
		}
	}
	//destra
	if(cX < 6 && ris == 0 && sol[cX+1][cY] == 0)
	{
		if(cY > 0)
		{
			if(cY < 6)
			{
				if(!(muri[cX][cY-1] == 2 || muri[cX][cY] == 2))
				{
					sol[cX+1][cY] = 1;
					ris += ric(cX+1, cY, ris);
				}
			}
			else
			{
				if(muri[cX][cY-1] != 2)
				{
					sol[cX+1][cY] = 1;
					ris += ric(cX+1, cY, ris);
				}
			}
		}
		else
		{
			if(muri[cX][cY] != 2)
			{
				sol[cX+1][cY] = 1;
				ris += ric(cX+1, cY, ris);
			}
		}
	}
	//sinistra
	if(cX > 0 && ris == 0 && sol[cX-1][cY] == 0)
	{
		if(cY > 0)
		{
			if(cY < 6)
			{
				if(!(muri[cX-1][cY-1] == 2 || muri[cX-1][cY] == 2))
				{
					sol[cX-1][cY] = 1;
					ris += ric(cX-1, cY, ris);
				}
			}
			else
			{
				if(muri[cX-1][cY-1] != 2)
				{
					sol[cX-1][cY] = 1;
					ris += ric(cX-1, cY, ris);
				}
			}
		}
		else
		{
			if(muri[cX-1][cY] != 2)
			{
				sol[cX-1][cY] = 1;
				ris += ric(cX-1, cY, ris);
			}
		}
	}
	//su
	if(cY > 0 && ris == 0 && sol[cX][cY-1] == 0)
	{
		if(cX > 0)
		{
			if(cX < 6)
			{
				if(!(muri[cX-1][cY-1] == 1 || muri[cX][cY-1] == 1))
				{
					sol[cX][cY-1] = 1;
					ris += ric(cX, cY-1, ris);
				}
			}
			else
			{
				if(muri[cX-1][cY-1] != 1)
				{
					sol[cX][cY-1] = 1;
					ris += ric(cX, cY-1, ris);
				}
			}
		}
		else
		{
			if(muri[cX][cY-1] != 1)
			{
				sol[cX][cY-1] = 1;
				ris += ric(cX, cY-1, ris);
			}
		}
	}
	return ris;
}


void move(int pos)
{
	int i, j, cordX, cordY, ok, ris;
	char s[5]="";
	
	//prendo coordinate
	for(i=0; i<7; i++)
	{
		for(j=0; j<7; j++)
		{
			if(matrice[i][j] == turnoCorrente)
			{
				cordX = i;
				cordY = j;
			}
		}
	}
	if(modalita == 0)
	{
		//modalita player
		if(pos == 0)
		{
			//select
			if (!(cordinataX == cordX && cordinataY == cordY))
			{
				//controllo vincita
				if(turnoCorrente == 1 && cordY == 6)
				{
					win();
				}
				else if(turnoCorrente == 2 && cordY == 0)
				{
					win();
				}
				else
				{
					cambioTurno();
				}
			}
		}
		else if(pos == 1)
		{
			//down
			if(cordY<6)
			{
				if(matrice[cordX][cordY+1] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX][cordY+1] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer(cordX*30+14, (cordY+1)*30, White);
					}
					else
					{
						LCD_DrawPlayer(cordX*30+14, (cordY+1)*30, Red);
					}
				}
				else if(cordY+1 < 6 && matrice[cordX][cordY+2] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX][cordY+2] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer(cordX*30+14, (cordY+2)*30, White);
					}
					else
					{
						LCD_DrawPlayer(cordX*30+14, (cordY+2)*30, Red);
					}
				}
			}
		}
		else if(pos == 2)
		{
			//left
			if(cordX>0)
			{
				if(matrice[cordX-1][cordY] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX-1][cordY] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer((cordX-1)*30+14, cordY*30, White);
					}
					else
					{
						LCD_DrawPlayer((cordX-1)*30+14, cordY*30, Red);
					}
				}
				else if(cordX-1 > 0 && matrice[cordX-2][cordY] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX-2][cordY] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer((cordX-2)*30+14, cordY*30, White);
					}
					else
					{
						LCD_DrawPlayer((cordX-2)*30+14, cordY*30, Red);
					}
				}
			}
		}
		else if(pos == 3)
		{
			//right
			if(cordX<6)
			{
				if(matrice[cordX+1][cordY] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX+1][cordY] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer((cordX+1)*30+14, cordY*30, White);
					}
					else
					{
						LCD_DrawPlayer((cordX+1)*30+14, cordY*30, Red);
					}
				}
				else if(cordX+1 < 6 && matrice[cordX+2][cordY] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX+2][cordY] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer((cordX+2)*30+14, cordY*30, White);
					}
					else
					{
						LCD_DrawPlayer((cordX+2)*30+14, cordY*30, Red);
					}
				}
			}
		}
		else if(pos == 4)
		{
			//up
			if(cordY>0)
			{
				if(matrice[cordX][cordY-1] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX][cordY-1] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer(cordX*30+14, (cordY-1)*30, White);
					}
					else
					{
						LCD_DrawPlayer(cordX*30+14, (cordY-1)*30, Red);
					}
				}
				else if(cordY-1 > 0 && matrice[cordX][cordY-2] == 3)
				{
					matrice[cordX][cordY] = 3;
					LCD_DrawPlayer(cordX*30+14, cordY*30, Yellow);
					matrice[cordX][cordY-2] = turnoCorrente;
					if (turnoCorrente == 1)
					{
						LCD_DrawPlayer(cordX*30+14, (cordY-2)*30, White);
					}
					else
					{
						LCD_DrawPlayer(cordX*30+14, (cordY-2)*30, Red);
					}
				}
			}
		}
	}
	else
	{
		//modalita muro
		if(pos == 0)
		{
			//select
			ok = 0;
			
			//controllo muro non sovrapposto
			if(muritempY < 5)
			{
				//down
				if(muri[muritempX][muritempY+1] == 2 && ruota == 2)
				{
					ok = 1;
				}
			}
			if(muritempX > 0)
			{
				//left
				if(muri[muritempX-1][muritempY] == 1 && ruota == 1)
				{
					ok = 1;
				}
			}
			if(muritempX < 5)
			{
				//right
				if(muri[muritempX+1][muritempY] == 1 && ruota == 1)
				{
					ok = 1;
				}
			}
			if(muritempY > 0)
			{
				//up
				if(muri[muritempX][muritempY-1] == 2 && ruota == 2)
				{
					ok = 1;
				}
			}
			if(muri[muritempX][muritempY] != 0)
			{
				//corrente
				ok = 1;
			}
			if(ok == 0)
			{
 				//controllo che esiste un percorso per vincere
				for(i=0; i<7; i++)
				{
					for(j=0; j<7; j++)
					{
						sol[i][j] = 0;
					}
				}
				ris = 0;
				muri[muritempX][muritempY] = ruota;
				sol[0][0] = 1;
				ris = ric(0, 0, ris);
				if(turnoCorrente == 1 && muriP1 > 0 && ris > 0)
				{
					LCD_DrawWall(muritempX*30+14, muritempY*30, Green, ruota);
					muriP1--;
					sprintf(s, "%d", muriP1);
					GUI_Text(35, 270, (uint8_t *) s, Black, Blue2);
					cambioTurno();
				}
				else if(turnoCorrente == 2 && muriP2 > 0 && ris > 0)
				{
					LCD_DrawWall(muritempX*30+14, muritempY*30, Green, ruota);
					muriP2--;
					sprintf(s, "%d", muriP2);
					GUI_Text(195, 270, (uint8_t *) s, Black, Blue2);
					cambioTurno();
				}
				else
				{
					muri[muritempX][muritempY] = 0;
				}
			}
		}
		else if(pos == 1)
		{
			//down
			if(muritempY < 5)
			{
				LCD_DrawWall(muritempX*30+14, muritempY*30, Blue2, ruota);
				if(muritempX > 0)
				{
					//left
					if(muri[muritempX-1][muritempY] != 0)
					{
						LCD_DrawWall((muritempX-1)*30+14, muritempY*30, Green, muri[muritempX-1][muritempY]);
					}
				}
				if(muritempX < 5)
				{
					//right
					if(muri[muritempX+1][muritempY] != 0)
					{
						LCD_DrawWall((muritempX+1)*30+14, muritempY*30, Green, muri[muritempX+1][muritempY]);
					}
				}
				if(muritempY > 0)
				{
					//up
					if(muri[muritempX][muritempY-1] != 0)
					{
						LCD_DrawWall(muritempX*30+14, (muritempY-1)*30, Green, muri[muritempX][muritempY-1]);
					}
				}
				if(muri[muritempX][muritempY] != 0)
				{
					//corrente
					LCD_DrawWall(muritempX*30+14, muritempY*30, Green, muri[muritempX][muritempY]);
				}
				muritempY = muritempY + 1;
				LCD_DrawWall(muritempX*30+14, muritempY*30, Cyan, ruota);
			}
		}
		else if(pos == 2)
		{
			//left
			if(muritempX > 0)
			{
				LCD_DrawWall(muritempX*30+14, muritempY*30, Blue2, ruota);
				if(muritempY < 5)
				{
					//down
					if(muri[muritempX][muritempY+1] != 0)
					{
						LCD_DrawWall(muritempX*30+14, (muritempY+1)*30, Green, muri[muritempX][muritempY+1]);
					}
				}
				if(muritempX < 5)
				{
					//right
					if(muri[muritempX+1][muritempY] != 0)
					{
						LCD_DrawWall((muritempX+1)*30+14, muritempY*30, Green, muri[muritempX+1][muritempY]);
					}
				}
				if(muritempY > 0)
				{
					//up
					if(muri[muritempX][muritempY-1] != 0)
					{
						LCD_DrawWall(muritempX*30+14, (muritempY-1)*30, Green, muri[muritempX][muritempY-1]);
					}
				}
				if(muri[muritempX][muritempY] != 0)
				{
					//corrente
					LCD_DrawWall(muritempX*30+14, muritempY*30, Green, muri[muritempX][muritempY]);
				}
				muritempX = muritempX - 1;
				LCD_DrawWall(muritempX*30+14, muritempY*30, Cyan, ruota);
			}
		}
		else if(pos == 3)
		{
			//right
			if(muritempX < 5)
			{
				LCD_DrawWall(muritempX*30+14, muritempY*30, Blue2, ruota);
				if(muritempY < 5)
				{
					//down
					if(muri[muritempX][muritempY+1] != 0)
					{
						LCD_DrawWall(muritempX*30+14, (muritempY+1)*30, Green, muri[muritempX][muritempY+1]);
					}
				}
				if(muritempX > 0)
				{
					//left
					if(muri[muritempX-1][muritempY] != 0)
					{
						LCD_DrawWall((muritempX-1)*30+14, muritempY*30, Green, muri[muritempX-1][muritempY]);
					}
				}
				if(muritempY > 0)
				{
					//up
					if(muri[muritempX][muritempY-1] != 0)
					{
						LCD_DrawWall(muritempX*30+14, (muritempY-1)*30, Green, muri[muritempX][muritempY-1]);
					}
				}
				if(muri[muritempX][muritempY] != 0)
				{
					//corrente
					LCD_DrawWall(muritempX*30+14, muritempY*30, Green, muri[muritempX][muritempY]);
				}
				muritempX = muritempX + 1;
				LCD_DrawWall(muritempX*30+14, muritempY*30, Cyan, ruota);
			}
		}
		else if(pos == 4)
		{
			//up
			if(muritempY > 0)
			{
				LCD_DrawWall(muritempX*30+14, muritempY*30, Blue2, ruota);
				if(muritempY < 5)
				{
					//down
					if(muri[muritempX][muritempY+1] != 0)
					{
						LCD_DrawWall(muritempX*30+14, (muritempY+1)*30, Green, muri[muritempX][muritempY+1]);
					}
				}
				if(muritempX > 0)
				{
					//left
					if(muri[muritempX-1][muritempY] != 0)
					{
						LCD_DrawWall((muritempX-1)*30+14, muritempY*30, Green, muri[muritempX-1][muritempY]);
					}
				}
				if(muritempX < 5)
				{
					//right
					if(muri[muritempX+1][muritempY] != 0)
					{
						LCD_DrawWall((muritempX+1)*30+14, muritempY*30, Green, muri[muritempX+1][muritempY]);
					}
				}
				if(muri[muritempX][muritempY] != 0)
				{
					//corrente
					LCD_DrawWall(muritempX*30+14, muritempY*30, Green, muri[muritempX][muritempY]);
				}
				muritempY = muritempY - 1;
				LCD_DrawWall(muritempX*30+14, muritempY*30, Cyan, ruota);
			}
		}
	}
}


void win()
{
	char s[30]="";
	int i;
	
	sprintf(s, "    P%d ha vinto!    ", turnoCorrente);
	GUI_Text(40, 300, (uint8_t *) s, Black, Blue2);
	
	if(multiplayer == 1 && turnoCorrente == id)
	{
		CAN_TxMsg.data[0] = (turnoCorrente & 0xFF);
		CAN_TxMsg.data[1] = ((modalita & 0xF0) << 4) + (ruota & 0x0F);
		if(modalita == 0)
		{
			CAN_TxMsg.data[2] = cordinataX & 0xFF;
			CAN_TxMsg.data[3] = cordinataY & 0xFF;
		}
		else
		{
			CAN_TxMsg.data[2] = muritempX & 0xFF;
			CAN_TxMsg.data[3] = muritempY & 0xFF;
		}
		CAN_TxMsg.len = 4;
		CAN_TxMsg.id = turnoCorrente;
		CAN_TxMsg.format = STANDARD_FORMAT;
		CAN_TxMsg.type = DATA_FRAME;
		CAN_wrMsg (1, &CAN_TxMsg);
	}
	
	//aspetto un po
	for(i=0; i<100000; i++)
	{
		
	}
	
	menu = 1;
	
	//abilito INT0
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT0_IRQn);
	LPC_SC->EXTINT &= (1 << 0);
}

void lose()
{
	char s[30]="";
	int i;
	
	sprintf(s, "    P%d hai perso!   ", turnoCorrente);
	GUI_Text(40, 300, (uint8_t *) s, Black, Blue2);
	
	
	//aspetto un po
	for(i=0; i<100000; i++)
	{
		
	}
	
	menu = 1;
	
	//abilito INT0
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT0_IRQn);
	LPC_SC->EXTINT &= (1 << 0);
}

void menu_init(void)
{
	NVIC_DisableIRQ(EINT0_IRQn);
	menu = 1;
	gameStart = 0;
	
	//schermo
	LCD_Clear(Blue2); 
	GUI_Text(80, 45, (uint8_t *) "select the", Black, Blue2);
	GUI_Text(80, 65, (uint8_t *) "GAME MODE", Black, Blue2);
	LCD_DrawLine(40, 100, 200, 100, Red);
	LCD_DrawLine(40, 140, 200, 140, Red);
	LCD_DrawLine(40, 100, 40, 140, Red);
	LCD_DrawLine(200, 100, 200, 140, Red);
	GUI_Text(70, 110, (uint8_t *) "Single board", Black, Blue2);
	LCD_DrawLine(40, 180, 200, 180, Black);
	LCD_DrawLine(40, 220, 200, 220, Black);
	LCD_DrawLine(40, 180, 40, 220, Black);
	LCD_DrawLine(200, 180, 200, 220, Black);
	GUI_Text(70, 190, (uint8_t *) "Two boards", Black, Blue2);
	
	//seleziono modalita single default
	multiplayer = 0;
	npc = 0;
	
}

void menuSingle(void)
{
	menu = 2;
	
	//schermo
	LCD_Clear(Blue2); 
	GUI_Text(50, 45, (uint8_t *) "Single board: select", Black, Blue2);
	GUI_Text(50, 65, (uint8_t *) "the opposite player", Black, Blue2);
	LCD_DrawLine(40, 100, 200, 100, Red);
	LCD_DrawLine(40, 140, 200, 140, Red);
	LCD_DrawLine(40, 100, 40, 140, Red);
	LCD_DrawLine(200, 100, 200, 140, Red);
	GUI_Text(70, 110, (uint8_t *) "   Human   ", Black, Blue2);
	LCD_DrawLine(40, 180, 200, 180, Black);
	LCD_DrawLine(40, 220, 200, 220, Black);
	LCD_DrawLine(40, 180, 40, 220, Black);
	LCD_DrawLine(200, 180, 200, 220, Black);
	GUI_Text(70, 190, (uint8_t *) "    NPC    ", Black, Blue2);
}

void menuMulti(void)
{
	menu = 3;
	
	//schermo
	LCD_Clear(Blue2); 
	GUI_Text(50, 45, (uint8_t *) "Two boards: select", Black, Blue2);
	GUI_Text(50, 65, (uint8_t *) "your player", Black, Blue2);
	LCD_DrawLine(40, 100, 200, 100, Red);
	LCD_DrawLine(40, 140, 200, 140, Red);
	LCD_DrawLine(40, 100, 40, 140, Red);
	LCD_DrawLine(200, 100, 200, 140, Red);
	GUI_Text(70, 110, (uint8_t *) "   Human   ", Black, Blue2);
	LCD_DrawLine(40, 180, 200, 180, Black);
	LCD_DrawLine(40, 220, 200, 220, Black);
	LCD_DrawLine(40, 180, 40, 220, Black);
	LCD_DrawLine(200, 180, 200, 220, Black);
	GUI_Text(70, 190, (uint8_t *) "    NPC    ", Black, Blue2);
}

void menuGestisci(int azione)
{
	if(azione == 0)
	{
		//select
		if(menu == 1)
		{
			if(multiplayer == 0)
			{
				menuSingle();
			}
			else
			{
				menuMulti();
			}
		}
		else if(menu == 2)
		{
			//singleplayer
			gameStart=1;
		}
		else if (menu == 3)
		{
			//multiplayer
			gameStart=1;
		}
	}
	else if(azione == 1)
	{
		//down
		if(menu == 1)
		{
			if(multiplayer == 0)
			{
				LCD_DrawLine(40, 100, 200, 100, Black);
				LCD_DrawLine(40, 140, 200, 140, Black);
				LCD_DrawLine(40, 100, 40, 140, Black);
				LCD_DrawLine(200, 100, 200, 140, Black);
				LCD_DrawLine(40, 180, 200, 180, Red);
				LCD_DrawLine(40, 220, 200, 220, Red);
				LCD_DrawLine(40, 180, 40, 220, Red);
				LCD_DrawLine(200, 180, 200, 220, Red);
				multiplayer = 1;
			}
		}
		else
		{
			if(npc == 0)
			{
				LCD_DrawLine(40, 100, 200, 100, Black);
				LCD_DrawLine(40, 140, 200, 140, Black);
				LCD_DrawLine(40, 100, 40, 140, Black);
				LCD_DrawLine(200, 100, 200, 140, Black);
				LCD_DrawLine(40, 180, 200, 180, Red);
				LCD_DrawLine(40, 220, 200, 220, Red);
				LCD_DrawLine(40, 180, 40, 220, Red);
				LCD_DrawLine(200, 180, 200, 220, Red);
				npc = 1;
			}
		}
	}
	else
	{
		//up
		if(menu == 1)
		{
			if(multiplayer == 1)
			{
				LCD_DrawLine(40, 100, 200, 100, Red);
				LCD_DrawLine(40, 140, 200, 140, Red);
				LCD_DrawLine(40, 100, 40, 140, Red);
				LCD_DrawLine(200, 100, 200, 140, Red);
				LCD_DrawLine(40, 180, 200, 180, Black);
				LCD_DrawLine(40, 220, 200, 220, Black);
				LCD_DrawLine(40, 180, 40, 220, Black);
				LCD_DrawLine(200, 180, 200, 220, Black);
				multiplayer = 0;
			}
		}
		else
		{
			if(npc == 1)
			{
				LCD_DrawLine(40, 100, 200, 100, Red);
				LCD_DrawLine(40, 140, 200, 140, Red);
				LCD_DrawLine(40, 100, 40, 140, Red);
				LCD_DrawLine(200, 100, 200, 140, Red);
				LCD_DrawLine(40, 180, 200, 180, Black);
				LCD_DrawLine(40, 220, 200, 220, Black);
				LCD_DrawLine(40, 180, 40, 220, Black);
				LCD_DrawLine(200, 180, 200, 220, Black);
				npc = 0;
			}
		}
	}
}

void NPCturno()
{
	//logica NPC
	int mossaEffettuata = 0;
	int i, j, cp1X, cp1Y;
	char s[5]="";
	
	//decido se muovermi
	if(muriP2 <= muriP1)
	{
		//cancello precedente
		LCD_DrawPlayer(cordinataX*30+14, cordinataY*30, Blue2);
		matrice[cordinataX][cordinataY] = 0;
		
		//guardo sopra
		if(cordinataY>0 && mossaEffettuata == 0)
		{
			if(cordinataX > 0)
			{
				if(cordinataX < 5)
				{
					if(!(muri[cordinataX-1][cordinataY-1] == 1 || muri[cordinataX][cordinataY-1] == 1))
					{
						if (matrice[cordinataX][cordinataY-1] == 0)
						{
							LCD_DrawPlayer(cordinataX*30+14, (cordinataY-1)*30, Red);
							matrice[cordinataX][cordinataY-1] = 2;
							mossaEffettuata = 1;
							cordinataY--;
						}
						else if(cordinataY-1 > 0)
						{
							if(!(muri[cordinataX-1][cordinataY-2] == 1 || muri[cordinataX][cordinataY-2] == 1))
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY-2)*30, Red);
								matrice[cordinataX][cordinataY-2] = 2;
								mossaEffettuata = 1;
								cordinataY = cordinataY - 2;
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX-1][cordinataY-1] != 1)
					{
						if (matrice[cordinataX][cordinataY-1] == 0)
						{
							LCD_DrawPlayer(cordinataX*30+14, (cordinataY-1)*30, Red);
							matrice[cordinataX][cordinataY-1] = 2;
							mossaEffettuata = 1;
							cordinataY--;
						}
						else if(cordinataY-1 > 0)
						{
							if(muri[cordinataX-1][cordinataY-2] != 1)
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY-2)*30, Red);
								matrice[cordinataX][cordinataY-2] = 2;
								mossaEffettuata = 1;
								cordinataY = cordinataY - 2;
							}
						}
					}
				}
			}
			else
			{
				if(muri[cordinataX][cordinataY-1] != 1)
				{
					if (matrice[cordinataX][cordinataY-1] == 0)
					{
						LCD_DrawPlayer(cordinataX*30+14, (cordinataY-1)*30, Red);
						matrice[cordinataX][cordinataY-1] = 2;
						mossaEffettuata = 1;
						cordinataY--;
					}
					else if(cordinataY-1 > 0)
					{
						if(muri[cordinataX][cordinataY-2] != 1)
						{
							LCD_DrawPlayer(cordinataX*30+14, (cordinataY-2)*30, Red);
							matrice[cordinataX][cordinataY-2] = 2;
							mossaEffettuata = 1;
							cordinataY = cordinataY - 2;
						}
					}
				}
			}
		}
		//guardo sinistra
		if(cordinataX>0 && mossaEffettuata == 0)
		{
			if(cordinataY > 0)
			{
				if(cordinataY < 5)
				{
					if(!(muri[cordinataX-1][cordinataY-1] == 2 || muri[cordinataX-1][cordinataY] == 2))
					{
						if (matrice[cordinataX-1][cordinataY] == 0)
						{
							LCD_DrawPlayer((cordinataX-1)*30+14, cordinataY*30, Red);
							matrice[cordinataX-1][cordinataY] = 2;
							mossaEffettuata = 1;
							cordinataX--;
						}
						else if(cordinataX-1 > 0)
						{
							if(!(muri[cordinataX-2][cordinataY-1] == 2 || muri[cordinataX-2][cordinataY] == 2))
							{
								LCD_DrawPlayer((cordinataX-2)*30+14, cordinataY*30, Red);
								matrice[cordinataX-2][cordinataY] = 2;
								mossaEffettuata = 1;
								cordinataX = cordinataX - 2;
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX-1][cordinataY-1] != 2)
					{
						if (matrice[cordinataX-1][cordinataY] == 0)
						{
							LCD_DrawPlayer((cordinataX-1)*30+14, cordinataY*30, Red);
							matrice[cordinataX-1][cordinataY] = 2;
							mossaEffettuata = 1;
							cordinataX--;
						}
						else if(cordinataX-1 > 0)
						{
							if(muri[cordinataX-2][cordinataY-1] != 2)
							{
								LCD_DrawPlayer((cordinataX-2)*30+14, cordinataY*30, Red);
								matrice[cordinataX-2][cordinataY] = 2;
								mossaEffettuata = 1;
								cordinataX = cordinataX - 2;
							}
						}
					}
				}
			}
			else
			{
				if(muri[cordinataX-1][cordinataY] != 2)
				{
					if (matrice[cordinataX-1][cordinataY] == 0)
					{
						LCD_DrawPlayer((cordinataX-1)*30+14, cordinataY*30, Red);
						matrice[cordinataX-1][cordinataY] = 2;
						mossaEffettuata = 1;
						cordinataX--;
					}
					else if(cordinataX-1 > 0)
					{
						if(muri[cordinataX-2][cordinataY] != 2)
						{
							LCD_DrawPlayer((cordinataX-2)*30+14, cordinataY*30, Red);
							matrice[cordinataX-2][cordinataY] = 2;
							mossaEffettuata = 1;
							cordinataX = cordinataX - 2;
						}
					}
				}
			}
		}
		//guardo destra
		if(cordinataX<6 && mossaEffettuata == 0)
		{
			if(cordinataY > 0)
			{
				if(cordinataY < 5)
				{
					if(!(muri[cordinataX][cordinataY-1] == 2 || muri[cordinataX][cordinataY] == 2))
					{
						if (matrice[cordinataX+1][cordinataY] == 0)
						{
							LCD_DrawPlayer((cordinataX+1)*30+14, cordinataY*30, Red);
							matrice[cordinataX+1][cordinataY] = 2;
							mossaEffettuata = 1;
							cordinataX++;
						}
						else if(cordinataX+1 < 6)
						{
							if(!(muri[cordinataX+1][cordinataY-1] == 2 || muri[cordinataX+1][cordinataY] == 2))
							{
								LCD_DrawPlayer((cordinataX+2)*30+14, cordinataY*30, Red);
								matrice[cordinataX+2][cordinataY] = 2;
								mossaEffettuata = 1;
								cordinataX = cordinataX + 2;
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX][cordinataY-1] != 2)
					{
						if (matrice[cordinataX+1][cordinataY] == 0)
						{
							LCD_DrawPlayer((cordinataX+1)*30+14, cordinataY*30, Red);
							matrice[cordinataX+1][cordinataY] = 2;
							mossaEffettuata = 1;
							cordinataX++;
						}
						else if(cordinataX+1 < 6)
						{
							if(muri[cordinataX+1][cordinataY-1] != 2)
							{
								LCD_DrawPlayer((cordinataX+2)*30+14, cordinataY*30, Red);
								matrice[cordinataX+2][cordinataY] = 2;
								mossaEffettuata = 1;
								cordinataX = cordinataX + 2;
							}
						}
					}
				}
			}
			else
			{
				if(muri[cordinataX][cordinataY] != 2)
				{
					if (matrice[cordinataX+1][cordinataY] == 0)
					{
						LCD_DrawPlayer((cordinataX+1)*30+14, cordinataY*30, Red);
						matrice[cordinataX+1][cordinataY] = 2;
						mossaEffettuata = 1;
						cordinataX++;
					}
					else if(cordinataX+1 < 6)
					{
						if(muri[cordinataX+1][cordinataY] != 2)
						{
							LCD_DrawPlayer((cordinataX+2)*30+14, cordinataY*30, Red);
							matrice[cordinataX+2][cordinataY] = 2;
							mossaEffettuata = 1;
							cordinataX = cordinataX + 2;
						}
					}
				}
			}
		}
		//guardo sotto
		if(cordinataY<6 && mossaEffettuata == 0)
		{
			if(cordinataX > 0)
			{
				if(cordinataX < 5)
				{
					if(!(muri[cordinataX-1][cordinataY] == 1 || muri[cordinataX][cordinataY] == 1))
					{
						if (matrice[cordinataX][cordinataY+1] == 0)
						{
							LCD_DrawPlayer(cordinataX*30+14, cordinataY*30+30, Red);
							matrice[cordinataX][cordinataY+1] = 2;
							mossaEffettuata = 1;
							cordinataY++;
						}
						else if(cordinataY+1 < 6)
						{
							if(!(muri[cordinataX-1][cordinataY+1] == 1 || muri[cordinataX][cordinataY+1] == 1))
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY+2)*30, Red);
								matrice[cordinataX][cordinataY+2] = 2;
								mossaEffettuata = 1;
								cordinataY = cordinataY + 2;
							}
						}
					}
				}
				else
				{
					if(muri[cordinataX-1][cordinataY] != 1)
					{
						if (matrice[cordinataX][cordinataY+1] == 0)
						{
							LCD_DrawPlayer(cordinataX*30+14, cordinataY*30+30, Red);
							matrice[cordinataX][cordinataY+1] = 2;
							mossaEffettuata = 1;
							cordinataY++;
						}
						else if(cordinataY+1 < 6)
						{
							if(muri[cordinataX-1][cordinataY+1] != 1)
							{
								LCD_DrawPlayer(cordinataX*30+14, (cordinataY+2)*30, Red);
								matrice[cordinataX][cordinataY+2] = 2;
								mossaEffettuata = 1;
								cordinataY = cordinataY + 2;
							}
						}
					}
				}
			}
			else
			{
				if(muri[cordinataX][cordinataY] != 1)
				{
					if (matrice[cordinataX][cordinataY+1] == 0)
					{
						LCD_DrawPlayer(cordinataX*30+14, cordinataY*30+30, Red);
						matrice[cordinataX][cordinataY+1] = 2;
						mossaEffettuata = 1;
						cordinataY++;
					}
					else if(cordinataY+1 < 6)
					{
						if(muri[cordinataX][cordinataY+1] != 1)
						{
							LCD_DrawPlayer(cordinataX*30+14, (cordinataY+2)*30, Red);
							matrice[cordinataX][cordinataY+2] = 2;
							mossaEffettuata = 1;
							cordinataY = cordinataY + 2;
						}
					}
				}
			}
		}
	}
	else
	{
		//piazzo muro
		for(i=0; i<7; i++)
		{
			for(j=0; j<7; j++)
			{
				if(matrice[i][j] == 1)
				{
					cp1X = i;
					cp1Y = j;
				}
			}
		}
		
		if(cp1X > 0)
		{
			if(cp1X < 5)
			{
				if(muri[cp1X][cp1Y] == 0 && muri[cp1X-1][cp1Y] != 1 && muri[cp1X+1][cp1Y] != 1)
				{
					i = cp1X;
					j = cp1Y;
					ruota = 1;
					mossaEffettuata = 1;
				}
				else if(muri[cp1X][cp1Y] == 0 && muri[cp1X-1][cp1Y] != 2 && muri[cp1X+1][cp1Y] != 2)
				{
					i = cp1X;
					j = cp1Y;
					ruota = 2;
					mossaEffettuata = 1;
				}
				if(cp1X > 1 && mossaEffettuata == 0)
				{
					if(muri[cp1X-1][cp1Y] == 0 && muri[cp1X-2][cp1Y] != 1 && muri[cp1X][cp1Y] != 1)
					{
						i = cp1X-1;
						j = cp1Y;
						ruota = 1;
						mossaEffettuata = 1;
					}
					else if(muri[cp1X-1][cp1Y] == 0 && muri[cp1X-2][cp1Y] != 2 && muri[cp1X][cp1Y] != 2)
					{
						i = cp1X-1;
						j = cp1Y;
						ruota = 2;
						mossaEffettuata = 1;
					}
				}
			}
			else
			{
				if(cp1X == 6)
				{
					if(muri[cp1X-1][cp1Y] == 0 && muri[cp1X-2][cp1Y] != 1)
					{
						i = cp1X-1;
						j = cp1Y;
						ruota = 1;
						mossaEffettuata = 1;
					}
					else if(muri[cp1X-1][cp1Y] == 0 && muri[cp1X-2][cp1Y] != 2)
					{
						i = cp1X-1;
						j = cp1Y;
						ruota = 2;
						mossaEffettuata = 1;
					}
				}
				else
				{
					if(muri[cp1X-1][cp1Y] == 0 && muri[cp1X-2][cp1Y] != 1 && muri[cp1X][cp1Y] != 1)
					{
						i = cp1X-1;
						j = cp1Y;
						ruota = 1;
						mossaEffettuata = 1;
					}
					else if(muri[cp1X-1][cp1Y] == 0 && muri[cp1X-2][cp1Y] != 2 && muri[cp1X][cp1Y] != 2)
					{
						i = cp1X-1;
						j = cp1Y;
						ruota = 2;
						mossaEffettuata = 1;
					}
				}
			}
		}
		else
		{
			if(muri[cp1X][cp1Y] == 0 && muri[cp1X+1][cp1Y] != 1)
			{
				i = cp1X;
				j = cp1Y;
				ruota = 1;
				mossaEffettuata = 1;
			}
			else if(muri[cp1X][cp1Y] == 0 && muri[cp1X+1][cp1Y] != 2)
			{
				i = cp1X;
				j = cp1Y;
				ruota = 2;
				mossaEffettuata = 1;
			}
		}
		
		if(mossaEffettuata == 1)
		{
			LCD_DrawWall(i*30+14, j*30, Green, ruota);
			muriP2--;
			muri[i][j] = ruota;
			sprintf(s, "%d", muriP2);
			GUI_Text(195, 270, (uint8_t *) s, Black, Blue2);
		}
		
		
		
	}
	
	//controllo vincita
	if(cordinataY == 0)
	{
		win();
	}
	else
	{
		cambioTurno();
	}
}
