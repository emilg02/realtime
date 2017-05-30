/* game2.c - xmain, prntr */
//test git
#include <conf.h>
#include <kernel.h>
#include <io.h>
#include <bios.h>
#include "header.h"
extern SYSCALL  sleept(int);
extern struct intmap far *sys_imp;
void drawCircle (int x, int y);
void drawCockpit();

#define ARROW_NUMBER 5
#define TARGET_NUMBER 4
unsigned char far *b800h; //define at the top
int receiver_pid;

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
 if ((scan == 46)&& (ascii == 3)) // Ctrl-C?
   asm INT 27; // terminate xinu

   send(receiver_pid, result); 

Skip1:

} // new_int9

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


typedef struct position
{
  int x;
  int y;

}  POSITION;

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
	while (1)
         {
               receive();
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


char display_draft[25][80];
POSITION target_pos[TARGET_NUMBER];
POSITION arrow_pos[ARROW_NUMBER];


void updateter()
{

  int i,j,x,y;
  int glider_position, glider_height;    
  char ch;



  glider_position = 39; //X axis (start in the middle)
  glider_height = 20; //marks the buttom of the glider
	
	/*Init screen*/
	for(i=0; i < 25; i++)
       for(j=0; j < 80; j++)
            display_draft[i][j] = ' ';  // blank
		
		
	drawCockpit();

  while(1)
  {

   receive();

   while(front != -1)
   {
     ch = ch_arr[front];
     if(front != rear)
       front++;
     else
       front = rear = -1;

     if ( (ch == 'a') || (ch == 'A') )
       if (glider_position >= 5 )
              glider_position--;
       else;
     else if ( (ch == 'd') || (ch == 'D') )
       if (glider_position <= 74 )
         glider_position++;
       else;
	   else if ( (ch =='w') || (ch == 'W') );
   } // while(front != -1)

     ch = 0;

	
	
	
	for (i=0; i<25;i++)
		for (j=0;j<80;j++)
			b800h[2*(i*80+j)] = display_draft[i][j];
		
		
    /*for(i=0; i < 25; i++)
      for(j=0; j < 80; j++)
        display[i*80+j] = display_draft[i][j];
    display[2000] = '\0';*/

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
        int uppid, dispid, recvpid,i;
		b800h = (unsigned char far*) 0xB8000000;
	
	asm {
		mov ah, 0
		mov al, 03
		int 10h
	}

	for (i=0;i<4000;i+=2)
 	{	
		b800h[i] = ' '; //ascii - empty character
		b800h[i+1] = 95; //attribute- 4 bits for background etc - color
	}
		
		
        resume( dispid = create(displayer, INITSTK, INITPRIO, "DISPLAYER", 0) );
        resume( recvpid = create(receiver, INITSTK, INITPRIO+3, "RECIVEVER", 0) );
        resume( uppid = create(updateter, INITSTK, INITPRIO, "UPDATER", 0) );
        receiver_pid =recvpid;  
        set_new_int9_newisr();
    schedule(2,10, dispid, 0,  uppid, 5);
} // xmain

void drawCircle (int x, int y)
{
	
	int i;
	for (i=x;i<x+4;i++)
		display_draft[y][i] = '*';
	
	display_draft[y+1][x-1] = '*';
	display_draft[y+1][x+4] = '*';
	
	display_draft[y+2][x-2] = '*';
	display_draft[y+2][x+5] = '*';
	
	display_draft[y+3][x-1] = '*';
	display_draft[y+3][x+4] = '*';
	
	for (i=x;i<x+4;i++)
		display_draft[y+4][i] = '*';
}

void drawCockpit()
{
	int i;
	drawCircle(FIRST_CIRCLE,CIRCLE_HEIGHT);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP, CIRCLE_HEIGHT);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP*2, CIRCLE_HEIGHT);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP*3, CIRCLE_HEIGHT);
	drawCircle(FIRST_CIRCLE+CIRCLE_GAP*4, CIRCLE_HEIGHT);
	
	display_draft[CIRCLE_HEIGHT][0] = '/';
	display_draft[CIRCLE_HEIGHT-1][1] = '/';
	display_draft[CIRCLE_HEIGHT-1][78] = '\\';
	display_draft[CIRCLE_HEIGHT][79] = '\\';
	
	for (i=3;i<77;i++)
		if (i< 39 || i > 41)
		display_draft[CIRCLE_HEIGHT-2][i] = '_';
	
	display_draft[CIRCLE_HEIGHT-2][38] = '/';
	display_draft[CIRCLE_HEIGHT-3][39] = '/';
	display_draft[CIRCLE_HEIGHT-3][40] = '\\';
	display_draft[CIRCLE_HEIGHT-2][41] = '\\';
	
	/*Draw circle descriptions*/
	display_draft[CIRCLE_HEIGHT-1][FIRST_CIRCLE] = 'U';
	display_draft[CIRCLE_HEIGHT-1][FIRST_CIRCLE+1] = 'P';
	/*End circle descriptions*/
	
		
}