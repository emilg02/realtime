/* game2.c - xmain, prntr */
#include <conf.h>
#include <kernel.h>
#include <io.h>
#include <bios.h>
#include "header.h"
extern SYSCALL  sleept(int);
extern struct intmap far *sys_imp;
void drawCircle (int x, int y,char color);
void drawCockpit();
void updateStrip();
void updateDistance();
unsigned char far *b800h; //define at the top
int receiver_pid;
int strip_row = 0;
char display_draft[25][80];

/*Global variables*/
int LEFT_DIRECTION = 0;
int RIGHT_DIRECTION = 0;
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

     if ( (ch == 'a' || ch == 'A'))
	 {
       LEFT_DIRECTION--;
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
     else if ( (ch == 'd') || (ch == 'D') )
	 {
       RIGHT_DIRECTION++;
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
	   else if ( (ch =='w') || (ch == 'W') )
	   {
		   updateStrip();
	   }
	   else if ( (ch =='s') || (ch == 'S'))
	   {
		   strip_row--;
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
	
	for (i=3;i<77;i++)
	{
		display_draft[CIRCLE_HEIGHT-2][i] = '_';
		b800h[2*((CIRCLE_HEIGHT-2)*80+i)+1] = 203;
	}

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
}

void updateStrip()
{
	int i;
		if (strip_row < CIRCLE_HEIGHT-1)
		{
		display_draft[strip_row][20-strip_row-RIGHT_DIRECTION-LEFT_DIRECTION] ='/';
		display_draft[strip_row][59+strip_row-RIGHT_DIRECTION-LEFT_DIRECTION] ='\\';
		for (i =20-strip_row-RIGHT_DIRECTION-LEFT_DIRECTION+1;
		i<59+strip_row-RIGHT_DIRECTION-LEFT_DIRECTION;i++)
		{
			//display_draft[strip_row][i+1] = 150;
			b800h[2*(strip_row*80+i)] = ' ';
			b800h[2*(strip_row*80+i)+1] = 10;
		}


		strip_row++;
		updateDistance();
	}
}

void updateDistance()
{
	display_draft[22][57] = 19 -strip_row + '0';
}