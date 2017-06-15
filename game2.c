/* game2.c - xmain, prntr */
#include <conf.h>
#include <kernel.h>
#include <io.h>
#include <bios.h>
#include "header.h"
#include <math.h>
extern SYSCALL  sleept(int);
extern struct intmap far *sys_imp;
void interrupt (*int8save)(void);
void drawCircle (int x, int y,char color);
void drawCockpit();
void updateStrip();
void updateDistance();
void updateLeftRightDirection();
void updateUpDownDirection();
void updateGPS();
void clearGPS();
void checkLanding();
void updateSpeed();
void decreaseSpeed();
void updateLength();
void updateLevelAndLives();
void generateWind();

void NoSound(void);
void Sound(int hertz);
void ChangeSpeaker(int status);
void mydelay(int n);
volatile int count;
unsigned char far *b800h; //define at the top
int receiver_pid;

char display_draft[25][80];
int uppid, dispid, recvpid;
/*Global variables*/
int LEFT_DIRECTION = 0;
int RIGHT_DIRECTION = 0;
volatile int DIRECTION = 0;
volatile int VERTICAL_DIRECTION=0;
int TMP_STRIP_DISTANCE =0;
int TMP_LAND_STRIP_DISTANCE=0;
int SPEED = 9;
int LEVEL = 0;
int STRIP_LOCATION[3] = {0, 3, 5};
int LIVES = 3;
int LANDING_COUNTER=0;
int strip_row = 0;
/*End global variables*/
INTPROC new_int9(int mdevno)
{
	char result = 0;
	int scan = 0;
	int ascii = 0;

	asm {
		MOV AH,1
		INT 16h
		JZ Skip1
		MOV AH,0
		INT 16h
		MOV BYTE PTR scan,AH
		MOV BYTE PTR ascii,AL
	} //asm
	if (scan == 75)
	result = 'a';
	else
	if (scan == 72)
	result = 'w';
	else
	if (scan == 77)
	result = 'd';
	else
	if (scan == 80)
	result = 's';
	else 
	if (scan == 45) //x - exit 
	{ 
		asm INT 27;
	}
	else
	if (scan == 79)
	result = 'e'; //END
	if ((scan == 46)&& (ascii == 3)) // Ctrl-C?
	asm INT 27; // terminate xinu

	send(receiver_pid, result); 

Skip1:

} // new_int9

void interrupt newint8(void)
{
	count++;
	asm {
		MOV AL,20h
		OUT 20h,AL
	}

} // newint8(void)

void set_new_int9_newisr()
{
	int i;
	for(i=0; i < 32; i++)
	if (sys_imp[i].ivec == 9)
	{
		sys_imp[i].newisr = new_int9;
		return;
	}

} // set_new_int9_newisr

void set_new_int8_newisr()
{
	int i;
	for(i=0; i < 32; i++)
	if (sys_imp[i].ivec == 8)
	{
		sys_imp[i].newisr = newint8;
		return;
	}

} 


char display[2001];

char ch_arr[2048];
int front = -1;
int rear = -1;
int point_in_cycle;
int gcycle_length;
int gno_of_pids;


/*------------------------------------------------------------------------
*  prntr  --  print a character indefinitely
*------------------------------------------------------------------------
*/



void displayer( void )
{
	int i,j;
	while (1)
	{
		receive();
		if (strip_row!=CIRCLE_HEIGHT)
		generateWind();
		for (i=0; i<25;i++)
		for (j=0;j<80;j++)
		b800h[2*(i*80+j)] = display_draft[i][j];
		//sleept(18);
		//printf(display);
	} //while
} // prntr

void receiver()
{
	while(1)
	{
		char temp;
		temp = receive();
		rear++;
		ch_arr[rear] = temp;
		if (front == -1)
		front = 0;
		//getc(CONSOLE);
	} // while

} //  receiver






void updateter()
{

	int i,j,x,y;
	char ch;
	
	/*Init screen*/
	for(i=0; i < 25; i++)
	for(j=0; j < 80; j++)
	display_draft[i][j] = ' ';  // blank	
	drawCockpit();
	while(1)
	{
		receive();
		while(front != -1 )
		{
			ch = ch_arr[front];
			if(front != rear)
			front++;
			else
			front = rear = -1;

			if ( (ch == 'a' || ch == 'A') && strip_row!=CIRCLE_HEIGHT )//&& 59+strip_row-DIRECTION<79)
			{
				LEFT_DIRECTION--;
				DIRECTION--;
				for (i = strip_row;i >=0;i--)
				{
					for (j=80;j>=0;j--)
					{
						if (display_draft[i][j] == '/' )
						{
							display_draft[i][j+1] = '/';
							display_draft[i][j] = ' ';			
						}
						else if (display_draft[i][j] == '\\')
						{
							display_draft[i][j+1] = '\\';
							display_draft[i][j] = ' ';
						}
						else if (b800h[2*(i*80+j)+1] == 10)
						{
							b800h[2*(i*80+j)+1] = 20;
							b800h[2*(i*80+j+1)+1] = 10;
						}
					}
				}
				
			}
			else if ( (ch == 'd' || ch == 'D')  && strip_row!=CIRCLE_HEIGHT)//&& 20-strip_row-DIRECTION>0 )
			{
				RIGHT_DIRECTION++;
				DIRECTION++;
				for (i=0; i<strip_row;i++)
				{
					for (j=0;j<80;j++)
					{
						if (display_draft[i][j] == '/')
						{
							display_draft[i][j-1] = '/';
							display_draft[i][j] = ' ';
							
						}
						else if (display_draft[i][j] == '\\')
						{
							display_draft[i][j-1] = '\\';
							display_draft[i][j] = ' ';
						}
						else if (b800h[2*(i*80+j)+1] == 10)
						{
							b800h[2*(i*80+j)+1] = 20;
							b800h[2*(i*80+j-1)+1] = 10;
						}
					}
				}
				
			}
			else if ( (ch =='w'|| ch == 'W') && strip_row!=CIRCLE_HEIGHT)
			{
				updateStrip();
				VERTICAL_DIRECTION++;
			}
			else if ( (ch =='s' || ch == 'S')&& strip_row!=CIRCLE_HEIGHT)
			{
				if (strip_row -1 > 0)
				{
					strip_row--;
					VERTICAL_DIRECTION--;
				}
			}
			else if ( (ch == 'e') || (ch == 'E'))
			{
				decreaseSpeed();
			}
		} // while(front != -1)
		updateStrip();  
		
		ch = 0;
	} // while(1)

} // updater 

int sched_arr_pid[5] = {-1};
int sched_arr_int[5] = {-1};


SYSCALL schedule(int no_of_pids, int cycle_length, int pid1, ...)
{
	int i;
	int ps;
	int *iptr;

	disable(ps);
	gcycle_length = cycle_length;
	point_in_cycle = 0;
	gno_of_pids = no_of_pids;

	iptr = &pid1;
	for(i=0; i < no_of_pids; i++)
	{
		sched_arr_pid[i] = *iptr;
		iptr++;
		sched_arr_int[i] = *iptr;
		iptr++;
	} // for
	restore(ps);

} // schedule 

xmain()
{
	int i;
	b800h = (unsigned char far*) 0xB8000000;
	srand();
	
	sound(80);
	mydelay(1);
	NoSound();
	asm {
		mov ah, 0
		mov al, 03
		int 10h
	}

	for (i=0;i<4000;i+=2)
	{	
		b800h[i] = ' '; //ascii - empty character
		b800h[i+1] = 20; //attribute- 4 bits for background etc - color
	}
	
	
	resume( dispid = create(displayer, INITSTK, INITPRIO, "DISPLAYER", 0) );
	resume( recvpid = create(receiver, INITSTK, INITPRIO+3, "RECIVEVER", 0) );
	resume( uppid = create(updateter, INITSTK, INITPRIO, "UPDATER", 0) );
	receiver_pid =recvpid;  
	set_new_int9_newisr();
	schedule(2,15, dispid, 0,  uppid, 12);
} // xmain

void drawCircle (int x, int y, char color)
{
	//display_draft will hold the char, b800h will hold the colors
	int i;
	for (i=x;i<x+4;i++)
	{
		display_draft[y][i] = '*';
		b800h[2*(y*80+i)+1] = color;
	}
	
	display_draft[y+1][x-1] = '*';
	display_draft[y+1][x+4] = '*';
	b800h[2*((y+1)*80+x-1)+1] = color;
	b800h[2*((y+1)*80+x+4)+1] = color;
	
	display_draft[y+2][x-2] = '*';
	display_draft[y+2][x+5] = '*';
	b800h[2*((y+2)*80+x-2)+1] = color;
	b800h[2*((y+2)*80+x+5)+1] = color;
	
	display_draft[y+3][x-1] = '*';
	display_draft[y+3][x+4] = '*';
	b800h[2*((y+3)*80+x-1)+1] = color;
	b800h[2*((y+3)*80+x+4)+1] = color;
	
	for (i=x;i<x+4;i++)
	{
		display_draft[y+4][i] = '*';
		b800h[2*((y+4)*80+i)+1] = color;
	}
}

void drawCockpit()
{
	int i,j;
	char updown[] = "UP/DOWN\0";
	char leftright [] = "LEFT/RIGHT\0";
	char gps[] = "GPS\0";
	char distance[] = "DISTANCE\0";
	char length[] = "LENGTH\0";
	
	
	drawCircle(FIRST_CIRCLE,CIRCLE_HEIGHT, 10);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP, CIRCLE_HEIGHT, 10);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP*2, CIRCLE_HEIGHT, 10);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP*3, CIRCLE_HEIGHT, 10);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP*4, CIRCLE_HEIGHT, 10);
	
	display_draft[CIRCLE_HEIGHT][0] = '/';
	display_draft[CIRCLE_HEIGHT-1][1] = '/';
	display_draft[CIRCLE_HEIGHT-1][78] = '\\';
	display_draft[CIRCLE_HEIGHT][79] = '\\';
	
	/*for (i=3;i<77;i++)
	{
		display_draft[CIRCLE_HEIGHT-2][i] = '_';
		//b800h[2*((CIRCLE_HEIGHT-2)*80+i)+1] = 3;
	}*/

	/*Draw circle descriptions*/

	for (i=0;i<strlen(updown);i++)
	display_draft[CIRCLE_HEIGHT-1][FIRST_CIRCLE+i] = updown[i];
	
	for (i=0;i<strlen(leftright);i++)
	display_draft[CIRCLE_HEIGHT-1][FIRST_CIRCLE+i+CIRCLE_GAP-2] = leftright[i]; 
	
	for (i=0;i<strlen(gps);i++)
	display_draft[CIRCLE_HEIGHT-1][FIRST_CIRCLE+i+CIRCLE_GAP*2+1] = gps[i]; 
	
	for (i=0;i<strlen(distance);i++)
	display_draft[CIRCLE_HEIGHT-1][FIRST_CIRCLE+i+CIRCLE_GAP*3-1] = distance[i]; 
	
	for (i=0;i<strlen(length);i++)
	display_draft[CIRCLE_HEIGHT-1][FIRST_CIRCLE+i+CIRCLE_GAP*4-2] = length[i]; 
	/*End circle descriptions*/
	
	for (i=20; i<25;i++)
	for (j=0;j<80;j++)
	if (display_draft[i][j] != '*')
	b800h[2*(i*80+j)+1] = 40;
	
	for (i=1;i<79;i++)
	{
		b800h[2*(19*80+i)+1] = 103;
	}
	
	/*Draw initial distance*/
	display_draft[22][56] = '2';
	display_draft[22][57] = '0';
	/*End initial distance*/
	
	updateLevelAndLives();
	
}

void updateStrip()
{
	int i,j, temp;
	temp = 0;
	if (strip_row < CIRCLE_HEIGHT)
	{	      
		for (i=0;temp<strip_row;i++)
		{
			for (j=20-temp-DIRECTION;j<=59+temp-DIRECTION;j++)
			{
				if (j == 20-temp-DIRECTION)
				b800h[2*(i*80+j)] = '/';
				if (j == 59+temp-DIRECTION)
				b800h[2*(i*80+j)] = '\\';
				b800h[2*(i*80+j)+1] = 10;
				
				
			}
			temp ++;
			
		}
		
		/*Clean sky*/
		for (i=0;i<=strip_row;i++)
		{
			for (j=59+temp-DIRECTION;j<80;j++)
			{
				b800h[2*(i*80+j)+1] = 20;
				b800h[2*(i*80+j)] = ' ';
			}
			for (j=20-temp-DIRECTION;j>=0;j--)
			{
				b800h[2*(i*80+j)+1] = 20;
				b800h[2*(i*80+j)] = ' ';
			}
		}
		/*End clean sky*/
		strip_row++;
		if (strip_row %3 == 0)
		VERTICAL_DIRECTION++;
		updateDistance();
		updateLeftRightDirection();
		updateUpDownDirection();
		updateGPS();

	}
	else
	checkLanding();
}

void updateDistance()
{	
	if (abs(strip_row) <= 10)
	{
		display_draft[22][56] = '1';
		display_draft[22][57] = 10 - abs(strip_row) + '0';
	}
	else
	{
		display_draft[22][56] = ' ';
		display_draft[22][57] = -2+ abs(strip_row) -TMP_STRIP_DISTANCE + '0';
		TMP_STRIP_DISTANCE+=2;
	}
	
}

void updateLeftRightDirection()
{
	if (DIRECTION + '0' < '0')
	{	
		display_draft[22][19] = '-';
	}
	else
	display_draft[22][19] = ' ';
	
	if (abs(DIRECTION) > 19)
	{
		if (DIRECTION < 0)
		display_draft[22][19]= '-';
		display_draft[22][20] = '1';
		display_draft[22][21] = '9';
	}
	else
	if (abs(DIRECTION) > 9)
	{
		display_draft[22][20] = '1';
		display_draft[22][21] = -10+ abs(DIRECTION) + '0';
	}
	else
	{
		display_draft[22][20] = ' ';
		display_draft[22][21] = abs(DIRECTION) + '0';
	}
}

void updateUpDownDirection()
{
	//display_draft[22][3] = strip_row + '0';
	if (VERTICAL_DIRECTION + '0' < '0')
	{	
		display_draft[22][1] = '-';
	}
	else
	display_draft[22][1] = ' ';

	if (VERTICAL_DIRECTION <- 19)
	{
		display_draft[22][1] = '-';
		display_draft[22][2] = '1';
		display_draft[22][3] = '9';
	}
	else
	if (VERTICAL_DIRECTION > 19)
	{
		display_draft[22][2] = '1';
		display_draft[22][3] = '9';
	}
	else
	if (abs(VERTICAL_DIRECTION) > 9)
	{
		display_draft[22][2] = '1';
		display_draft[22][3] = -10+ abs(VERTICAL_DIRECTION) + '0';
	}
	else
	{
		display_draft[22][2] = ' ';
		display_draft[22][3] = abs(VERTICAL_DIRECTION) + '0';
	}
}

void updateGPS()
{
	clearGPS();
	if (DIRECTION == 0) //show up arrow*/
	{
		display_draft[21][40] = '^';
		display_draft[22][40] = '|';
		display_draft[23][40] = '|';
	}
	else
	if (DIRECTION < 0) //right
	{
		display_draft[21][41] = '/';
		display_draft[22][40] = '/';
		display_draft[23][39] = '/';
	}
	else //LEFT
	{
		display_draft[21][39] = '\\';
		display_draft[22][40] = '\\';
		display_draft[23][41] = '\\';
	}
}

void clearGPS()
{

	int i;
	for (i=38;i<=41;i++){
		display_draft[21][i] = ' ';
		display_draft[22][i] = ' ';
		display_draft[23][i] = ' ';
		
	}
	
}

void checkLanding()
{
	int perfect_landing_x1, perfect_landing_y1, perfect_landing_x2,perfect_landing_y2,i,j;
	char endgame[] = "Congratulations, you finished the game!\0";
	char badendgame [] = "You ran out of lives! Try again!\0";
	perfect_landing_x1 = 18;
	perfect_landing_y1 = 2;
	perfect_landing_x2 = 18;
	perfect_landing_y2 = 77;
	if (strip_row == CIRCLE_HEIGHT) //if finished
	{
		if (b800h[2*(perfect_landing_x1*80+perfect_landing_y1)+1] == 10 &&
				b800h[2*(perfect_landing_x2*80+perfect_landing_y2)+1]  == 10)
		{
			display_draft[22][15] = SPEED + '0';
			if (LANDING_COUNTER < 19 && SPEED == 0)  //finished
			{
				/*Check if finished game*/
				
				if (LEVEL+1 == 3 && LIVES > 0) //game finished
				{
					sound(600);
					mydelay(1);
					NoSound();
					sound(800);
					mydelay(1);
					NoSound();
					
					b800h[2*(22*80+15)] ='0'; //set speed to 0
					for (i=0;i<strlen(endgame);i++)
					b800h[2*(12*80+20+i)] = endgame[i];
					exit(0);					
				}
				/*Level passed sound*/
					sound(400);
					mydelay(1);
					NoSound();
				/*Reset screen*/
				for (i=0;i<4000;i+=2)
				{	
					b800h[i] = ' '; //ascii - empty character
					b800h[i+1] = 20; //attribute- 4 bits for background etc - color
				}
				strip_row =0;
				LEVEL++;
				LEFT_DIRECTION = 0;
				RIGHT_DIRECTION = 0;
				DIRECTION = STRIP_LOCATION[LEVEL];
				VERTICAL_DIRECTION=0;
				TMP_STRIP_DISTANCE =0;
				TMP_LAND_STRIP_DISTANCE=0;
				LANDING_COUNTER=0;	
				SPEED = 9;
				resume( uppid = create(updateter, INITSTK, INITPRIO, "UPDATER", 0) );
				schedule(2,15-2*STRIP_LOCATION[LEVEL], dispid, 0,  uppid, 12-2*STRIP_LOCATION[LEVEL]);

				/*End reset screen*/
				
				
			}
			else
			{
				/*Reverse strip*/
				for (i=0;i<LANDING_COUNTER;i++)
				for (j=0;j<80;j++)
				b800h[2*(i*80+j)+1] = 20;
				LANDING_COUNTER++;
				updateSpeed();
				updateLength();
				/*End reverse strip*/
			}	
		}
		else //bad landing, not in strip
		{
			LIVES--;
				sound(80); //psila
				mydelay(1);
				NoSound();
			if (LIVES == 0) //END GAME
			{
									sound(30);
					mydelay(1);
					NoSound();
										sound(20);
					mydelay(1);
					NoSound();
				for (i=0;i<strlen(badendgame);i++)					
				b800h[2*(12*80+20+i)] = badendgame[i];
				exit(0);
			}
			/*Reset screen*/
			for (i=0;i<4000;i+=2)
			{	
				b800h[i] = ' '; //ascii - empty character
				b800h[i+1] = 20; //attribute- 4 bits for background etc - color
			}
			strip_row =0;
			LEFT_DIRECTION = 0;
			RIGHT_DIRECTION = 0;
			DIRECTION = STRIP_LOCATION[LEVEL];
			VERTICAL_DIRECTION=0;
			TMP_STRIP_DISTANCE =0;
			TMP_LAND_STRIP_DISTANCE=0;
			LANDING_COUNTER=0;	
			SPEED = 9;
			resume( uppid = create(updateter, INITSTK, INITPRIO, "UPDATER", 0) );
			schedule(2,15-2*STRIP_LOCATION[LEVEL], dispid, 0,  uppid, 12-2*STRIP_LOCATION[LEVEL]);

			/*End reset screen*/
		}
	}
	
}

void updateSpeed()
{
	int i,j;
	char speedStr[] = "Speed:";
	
	for (i=0;i<strlen(speedStr);i++)
	display_draft[22][i+8] = speedStr[i];
	
	display_draft[22][15] = SPEED + '0';
	b800h[2*(22*80+15)+1] = 155;
	
}

void decreaseSpeed()
{
	if (SPEED-1 >= 0)
	SPEED--;
}

void updateLength()
{
	
	if (abs(LANDING_COUNTER) <= 10)
	{
		display_draft[22][74] = '1';
		display_draft[22][75] = 10 - abs(LANDING_COUNTER) + '0';
	}
	else
	{
		display_draft[22][74] = ' ';
		display_draft[22][75] = -2+ abs(LANDING_COUNTER) -TMP_LAND_STRIP_DISTANCE + '0';
		TMP_LAND_STRIP_DISTANCE+=2;
	}
}

void updateLevelAndLives()
{
	int i;
	char level[] = "Level:";
	char lives[] = "Lives:";
	for (i=0;i<strlen(level);i++)
	display_draft[24][i+25] = level[i];
	display_draft[24][31] = LEVEL + '0';
	
	for (i=0;i<strlen(lives);i++)
	display_draft[24][i+43] = lives[i];
	display_draft[24][49] = LIVES + '0';
}

void generateWind()
{
	int wind_rand, wind_direction,i,j;
	wind_direction = rand() % 2;
	wind_rand = rand () %3;
	if (wind_rand == 1)
	{
		if (wind_direction)
		{
			DIRECTION+=1;
			for (i=0; i<strip_row;i++)
			{
				for (j=0;j<80;j++)
				{
					if (display_draft[i][j] == '/')
					{
						display_draft[i][j-1] = '/';
						display_draft[i][j] = ' ';
						
					}
					else if (display_draft[i][j] == '\\')
					{
						display_draft[i][j-1] = '\\';
						display_draft[i][j] = ' ';
					}
					else if (b800h[2*(i*80+j)+1] == 10)
					{
						b800h[2*(i*80+j)+1] = 20;
						b800h[2*(i*80+j-1)+1] = 10;
					}
				}
			}
			
		}			
		else
		{
			DIRECTION-=1;
			for (i = strip_row;i >=0;i--)
			{
				for (j=80;j>=0;j--)
				{
					if (display_draft[i][j] == '/' )
					{
						display_draft[i][j+1] = '/';
						display_draft[i][j] = ' ';			
					}
					else if (display_draft[i][j] == '\\')
					{
						display_draft[i][j+1] = '\\';
						display_draft[i][j] = ' ';
					}
					else if (b800h[2*(i*80+j)+1] == 10)
					{
						b800h[2*(i*80+j)+1] = 20;
						b800h[2*(i*80+j+1)+1] = 10;
					}
				}
			}
			
		}
	}
	
	
}

void mydelay(int n)
{
	asm {
		CLI
		PUSH AX
		MOV AL,036h
		OUT 43h,AL
		MOV AX,9700
		OUT 40h,AL
		MOV AL,AH
		OUT 40h,AL 
		POP AX
	} // asm

	//int8save = getvect(8);
	//setvect(8,newint8);
	asm  { STI};
	count = 0;
	while(count <= n*110)
	;

	asm {
		CLI
		PUSH AX
		MOV AL,036h
		OUT 43h,AL
		MOV AX,0
		OUT 40h,AL
		MOV AL,AH
		OUT 40h,AL 
		POP AX
	} // asm


	//setvect(8,int8save);


} //mydelay

/*------------------------------------------------
ChangeSpeaker - Turn speaker on or off. */

void ChangeSpeaker( int status )
{
	int portval;
	//   portval = inportb( 0x61 );

	portval = 0;
	asm {
		PUSH AX
		MOV AL,61h
		MOV byte ptr portval,AL
		POP AX
	}

	if ( status==ON )
	portval |= 0x03;
	else
	portval &=~ 0x03;
	// outportb( 0x61, portval );
	asm {
		PUSH AX
		MOV AX,portval
		OUT 61h,AL
		POP AX
	} // asm

} /*--ChangeSpeaker( )----------*/

void Sound( int hertz )
{
	unsigned divisor = 1193180L / hertz;

	ChangeSpeaker( ON );

	//        outportb( 0x43, 0xB6 );
	asm {
		PUSH AX
		MOV AL,0B6h
		OUT 43h,AL
		POP AX
	} // asm


	//       outportb( 0x42, divisor & 0xFF ) ;
	asm {
		PUSH AX
		MOV AX,divisor
		AND AX,0FFh
		OUT 42h,AL
		POP AX
	} // asm


	//        outportb( 0x42, divisor >> 8 ) ;

	asm {
		PUSH AX
		MOV AX,divisor
		MOV AL,AH
		OUT 42h,AL
		POP AX
	} // asm

} /*--Sound( )-----*/


void NoSound( void )
{
	ChangeSpeaker( OFF );
} /*--NoSound( )------*/


