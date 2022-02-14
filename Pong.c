//Pong
//Scott Snider
//Weber State
//revisions:
//04/26/21 | Scott | Version 1.0 completely functional game





#include <C8051F020.h>
#include <lcd.h>

unsigned long tacc = 0; //accumulator for pot 1
unsigned long pacc = 0; //accumulator for pot 2
const M = 256;			//constant for averaging pots
int rawpot2 = 0;		//averaged values for pots 1 and 2
int rawpot = 0;
int t_count = 0;			//count of samples take for pot1
int p_count = 0;			//count of samples take for pot2
long ADCdata = 0;		//holder for data from the ADC
int right_paddle = 32;	//y positions of each paddle
int left_paddle = 32;
long lsize;				//size of left paddle
unsigned int lpad_size = 2;	//mask used for lpaddle
long rsize;				//size of right paddle
unsigned int rpad_size = 2;	//mask used for right paddle
int speed_switches = 0;	//holds DIP switch values for the speed switches
int speed = 0;		//gameplay speed value
int angle = 0; 	  //angle 0 = Y2 X1; 1 = Y1 X2
int x_vel = 1;	//direction -1 = left or negative; 1 = right or positive
int y_vel = 1;	//1 = down; -1 - up
int ball_x = 5;	//ball x position
int ball_y = 5;	//ball y position
unsigned char Player_1 = 0;		//Player 1 and 2 scores
unsigned char Player_2 = 0;
code unsigned char sine[] = { 176, 217, 244, 254, 244, 217, 176, 128, 80, 39, 12, 2, 12, 39, 80, 128 };
//approximation of a sine wave

unsigned char phase = sizeof(sine)-1;	// current point in sine to output
unsigned int duration = 0;		// number of cycles left to output	
sbit BUTTON = P1^1;	 //define buttons


//Main function: start of program. initializes 8051 by disabling 
//watchdogs, enabling ports, enabling Interrupts for ADC and 3,
//setting Timer2 initial value for ADC.enabling Timer 1, 2, and 3 
//and using timer 1 to wait for external crystal to stabilize
//switching to external 22.1184mhz crystal
//initializing LCD
void main()
{
   P1 = 0xFF;
   WDTCN = 0xde;  // disable watchdog
   WDTCN = 0xad;
   XBR2 = 0x40;   // enable port output
   OSCXCN = 0x67; // turn on external crystal
   TMOD = 0x21;   // wait 1ms using T1 mode 2
   REF0CN = 3;
   EIE2 = 0x03;	  //enable interrupts for Timer 3 and ADC
   ADC0CN =0x8C;
   RCAP2H = 0xFF;   //0xFE
   RCAP2L = 0xA8;	//0x98
   T2CON = 0x00;    //setting timer 2 to autoreload
   TR2 = 1;			//enable timer 2
   IE = 0x80;		//enable all interrupts
   TH1 = -167;    // 2MHz clock, 167 counts - 1ms
   TR1 = 1;			//enable timer 1
   TL1 = 0;			//set timer 1 low to zero
   TR0 = 1;			//enable timer 0
   TL0 = 0;			//set timer 0 to 0
   TH0 = 0;
   while ( TF1 == 0 ) { }          // wait 1ms
   while ( !(OSCXCN & 0x80) ) { }  // wait till oscillator stable
   OSCICN = 8;    // switch over to 22.1184MHz
   init_lcd();		//initialize the LCD
   TMOD= 0x11;		//set timers 0 and 1 to 8 bit autoreload
   Player_1 = 0;	//set players scores to 0
   Player_2 = 0;
   TR1 = 1;			//start timer 1

   reset();			//reset the game

	for (;;)
	{
		blank_screen();		//blank the screen before redrawing
		get_size();			//get the size of the paddles and the gamespeed
		draw_edges();		//draw the top and bottom
		check_win();		//check if the ball has gone past either side
		draw_right();		//draw the right paddle and size it correctly
		draw_left();		//draw the left paddle and size it correctly
		move_ball();		//calculate and move the ball and detect collisions
		draw_ball();		//draw the ball


		while (TF0 == 0){}	//loop until timer 0 overflows
			TF0 = 0;		//clear timer 0 overflow
			TH0 = speed; //derived from dip switches and get_size() function
			TL0 = 0;
		refresh_screen(); //refresh screen with new data
	}

}


//Get_size is called each frame to get the DIP switch inputs for the paddle
// sizes for each player and the game speed
// Variables used are lpad_size, P1, lsize, rpad_size, rpad, speed_switches,
// and speed
void get_size (void)
{
   lpad_size = P1;					//get DIP switch values from P1
   lpad_size = lpad_size^0xFF;		//XOR them with FF to get the compliment
   lpad_size = lpad_size << 12;		//Shift them left 12 and then right 14 to 
   lpad_size = lpad_size >> 14;		//isolate the 2 bits for lpad
   if (lpad_size == 0)				//create lsize mask depending on lpad_size value
   		lsize = 0xFF;
   if (lpad_size == 1)
   		lsize = 0xFFF;
   if (lpad_size == 2)
   		lsize = 0xFFFF;
   if (lpad_size == 3)				//if lpad_size is 3 it is reset to 14 so that the
		lpad_size = 14;				//calculations in draw_left() are correct for full
									// height
   rpad_size = P1;					//get DIP switch values from P1
   rpad_size = rpad_size^0xFF;		//XOR with FF to get compliment
   rpad_size = rpad_size<< 10;		//shift left 10 and right 14 to isolate 
   rpad_size = rpad_size >> 14;		//2 bits for rpad
   if (rpad_size == 0)				//create rsize mask depending on rpad_size
   		rsize = 0xFF;
   if (rpad_size == 1)
   		rsize = 0xFFF;
   if (rpad_size == 2)
   		rsize = 0xFFFF;	
   if (rpad_size == 3)				//if rpad_size == 3 reset it to 14 for
		rpad_size = 14;				// proper calculations in draw_left()
		
   speed_switches = P1;						//get values from P1
   speed_switches = speed_switches^0xFF;	//get compliment
   speed_switches = speed_switches >> 6;	//isolate 2 bits for speed
	if (speed_switches == 0)				//depending on the DIP swithces
		speed = 0x88;						//set the speed value
	if (speed_switches == 1)				//speed is used in the main loop
		speed = 0x66;						// loaded into timer 0 for gamespeed
	if (speed_switches == 2)				//these numbers arent exact they 
		speed = 0x22;						//were chosen based on feel
	if (speed_switches == 3)
		speed = 0x00;
}



//used to write a number to the screen in base 10
//no external variables are used
void write_num (unsigned char row, unsigned char col, unsigned char num)
{
	write_char(row, col, num/10 + '0');			//get the ones digit of the number
	write_char(row, col + 6, num%10 + '0');	
}						//now get the tens digit of the number and move over 1 character	


//this is used to directly write a character to the screen
//font5x8[] and screen[]
void write_char (unsigned char row, unsigned char col, char ch)
{
	int i = row * 128 + col;	//get the array start offset
	int j = (ch - 32)*5;	//get the font array value by offsetting the ascii value by 32
	char k;
	for (k = 0; k < 5; k++, i++, j++)
	screen[i]= font5x8[j];	
}			//increment through the font memory and the screen space to write the character




//draws the top and bottom edges of the screen each frame
//no external variables are used
void draw_edges(void)
{
  int i = 0;
  for (i = 0; i < 128; i++)
	screen[i] = 1;						//draws a line across the top of the screen
  for (i = 7 * 128; i < 8*128; i++)
    screen[i]|= 0x80;					//draws a line across the bottom of the screen
}



//responsible for drawing the right side paddle
//external variables used are: screen[], rpad_size, right_paddle, rawpot2, rsize
void draw_right(void)
{
	char page;
	int i;
	long mask;
	if (rpad_size == 14)				//special case for full height paddle
	{
		for (i = 0; i < 128*8;i = i + 128)	//draws a line up the side of the screen
		{
			screen[i]|= 0xFF;
		}
		right_paddle = 0;		//sets the paddle location to zero so the ball always hits it
		return;
	}
	right_paddle = rawpot2/64;	//gets a value between 0 and 64 from the potentiometer input
	if ((right_paddle + rpad_size*4 + 8) > 64)
		right_paddle = 64 - (rpad_size*4 + 8);
	page = right_paddle/8;		//this gets the page the paddle starts on
	mask = rsize <<(right_paddle%8);//this gets the mask for the paddle to be applied to the screen
	i = page*128;				//128 because the screen is 128 pixels wide
	screen[i]|= mask;			//writes the first chunk of the paddle to the screen
	if (page < 7)
		screen[i+128]|= mask >> 8;	//if the paddle takes up 2 pages write to the next one
	if (page < 6)
		screen[i+256]|= mask >> 16;	//if the paddle takes up 3 pages write to the next one

}


//responsible for drawing the left side paddle
//external variables used are: screen[], lpad_size, left_paddle, rawpot, lsize
void draw_left(void)
{
	char page;
	int i;
	long mask;
	if (lpad_size == 14)				//special case for full height paddle
	{
		for (i = 127; i < 128*8;i = i + 128)		//draws a line up the side of the screen
		{
			screen[i]|= 0xFF;
		}
		left_paddle = 0;				//sets the paddle location to zero so the ball always hits it
		return;
	}
	left_paddle = rawpot/64;			//gets a value between 0 and 64 from the potentiometer input
	if ((left_paddle + lpad_size*4 + 8) > 64)
		left_paddle = 64 - (lpad_size*4 + 8);
	page = left_paddle/8;				//this gets the page the paddle starts on
	mask = lsize <<(left_paddle%8);		//this gets the mask for the paddle to be applied to the screen
	i = page*128 + 127;	//128 because the screen is 128 pixels wide then plus 127 to get to the other side of the screen
	screen[i]|= mask;					//writes the first chunk of the paddle to the screen
	if (page < 7)
		screen[i+128]|= mask >> 8;		//if the paddle takes up 2 pages write to the next one
	if (page < 6)
		screen[i+256]|= mask >> 16;		//if the paddle takes up 2 pages write to the next one
}



//this checks if the ball has gone past either sides paddle
//external variables used are TMR3CN, TMR3RLL, TMR3RLH, REF0CN, DAC0CN, duration, Player_1, ball_x, TH4, x_vel,
//ball_y, angle, y_vel, TL4, Player_2, TL1

void check_win(void)
{
	if (ball_x > 128)		//if the ball has gone off the right side of the screen give player 1 a point and reset
	{		
		TMR3CN = 0x06;	// timer 2, auto reload
		TMR3RLL = 0x00;	// these two numbers are chosen to make the DAC generate a specific frequency for this noise.
		TMR3RLH = 0xE9;
		REF0CN = 3;	// turn on voltage reference
		DAC0CN = 0x88; 	// update on timer 3, left justified
		TMR3CN = 0x06;	//start tuimer 3 clear reset run at system speed
		duration = 50;	//this is the number of cycles of sine waves the ADC generates
		Player_1++;		//increment player 1's score
		draw_score();	//draw score
		wait();			//display score for 2 seconds
		ball_x = 0 + (TH4/16);	//move the x position to a random spot determined by Timer 4's high byte amd divide by 16 to ensure its on the left 1/4 of the screen
		x_vel = 1;				//set the ball to be moving toward the player that missed
		ball_y = TL1/4;			//set the y velocity to random spot determined by Timer 1 low byte
		if (TH4 > 128)			//set the angle to a random value based on Time 4's high byte
			angle = 1;			// 1 is up or down 2 over 1
		else
			angle = 0;			//zero is up or down 1 and over 2. the directions are determined by x_vel and y_vel
		if (TL4 > 128)
			y_vel = 1;			//set y_vel to be random from Timer 4 low byte. if its -1 it moves up if its 1 it moves down
		else
			y_vel = -1;

	}
	if (ball_x < -1)		//same code as above but for when the ball goes off the left side of the screen
	{
		TMR3CN = 0x06;	// timer 2, auto reload
		TMR3RLL = 0x00;
		TMR3RLH = 0xE9;
		REF0CN = 3;	// turn on voltage reference
		DAC0CN = 0x88; 	// update on timer 2, left justified
		TMR3CN = 0x06;
		duration = 50;
		Player_2++;
		draw_score();
		wait();
		ball_x = 128 - (TH4/16);		//128 - Timer 4 high/16 ensures the ball is on the right 1/4 of the screen
		x_vel = -1;
		ball_y = TL1/4;
		if (TH4 > 128)
			angle = 1;
		else
			angle = 0;
		if (TL4 > 128)
			y_vel = 1;
		else
			y_vel = -1;
	}
	if (Player_1 == 11 || Player_2 == 11)		//if either player has reached 11 points the game is reset
	{
		reset();
	}
}



//resets scores for both players and calls the ready screen after printing ready
//external variables used: Player_1, Player_2, angle, ADC0H
//functions called: draw_score(), write_char(), Ready(), refresh_screen()

void reset(void)
{
	draw_score();
	Player_2 = 0;		//set scores to zero after  drawing final score
	Player_1 = 0;

	write_char(4,45,'R');	//print ready
	write_char(4,51,'E');
	write_char(4,58,'A');
	write_char(4,65,'D');
	write_char(4,72,'Y');
	refresh_screen();		//print screen

	Ready();
}


//waits for a button to be pressed before the game starts while calculating random positions for the ball at game start
//external variables used: RCAP4H, RCAP4L, BUTTON, TH4, ball_x, x_vel, angle, ball_y, y_vel

void Ready(void)
{
    RCAP4H = 0x00; 			//Timer 4 is loaded to run its entire range
    RCAP4L = 0x00;	//	47202
	T4CON = 0x04;		//start timer 4
	while(BUTTON)
	{
		if (TH4 < 128)		//depending on timer 4 high byte pick an x position for the ball
		{
			ball_x = 0 + (TH4/8);	//timer 4/8 ensures the ball is close to the left side
			x_vel = 1;				//1 sets the ball to move to the right
		}
		else
		{
			ball_x = 128 - (TH4-128)/8;		//128 - TH4-128/8 ensures the ball is close to the right side
			x_vel = -1;						//1 sets the ball to move to the left
		}
		ball_y = TL1/4;					//ball y is set to Timer 1 low byte/4 to get a position between 0 and 64
		if (TH4 > 128)					//depending on the Timer 4 high byte angle is set accordingly
			angle = 1;
		else
			angle = 0;
		if (TL4 > 128)					//y_vel is set to be up or down depending on timer 4 low byte
			y_vel = 1;
		else
			y_vel = -1;
	}
}



//used to erase the ball and paddles and then dislay the score
//functions used: blank_screen(), draw_edges(), write_num(), refresh_screen()
void draw_score(void)
{	
	blank_screen();				//clears screen
	draw_edges();				//redraws top and bottom edges
	write_num(1,45,Player_1);	//displays both scores seperated a little
	write_num(1,70,Player_2);
	refresh_screen();		//prints to the screen
}


//waits to seconds to display the score
//external variables used: RCAP4H, RCAP4L, T4CON

void wait(void)
{
	int waiter = 0;
    RCAP4H = 0xB8; 		//this value makes timer 4 overflow at 10 milliseconds
    RCAP4L = 0x62;	
	T4CON = 0x04;
	while (waiter < 200)		//overflows 200 times to make 2 seconds
	{
		while (T4CON == 0x04){}
		T4CON = 0x04;			//clears timer 4 overflow
		waiter++;
	}
}


//moves the ball each frame
//external variables used: angle, ball_x, ball_y, x_vel, y_vel

void move_ball(void)
{
	if (angle == 0)					//if the angle is 0 move hte ball over 1 and up or down 2 depending on the veloci
	{
		ball_x = ball_x + x_vel;
		ball_y = ball_y + y_vel*2;
	}
	if (angle == 1)
	{
		ball_x = ball_x + x_vel*2;	//if the angle is 1 move the ball over 2 and up or down to depending on the velocities
		ball_y = ball_y + y_vel;
	}
}


//draws the ball and checks if it has hit an edge or one of the paddles
//external variables used: ball_x, ball_y, duration, TMR3RLL, TMR3RLH, REF0CN, DAC0CN, TMR3CN, 
//y_vel, x_vel, left_paddle, lpad_size, right_paddle, rpad_size, angle, screen[]

void draw_ball(void)
{
	int x = ball_x;
	char page = ball_y/8;			//these 2 lines calculate the page the ball is on and a mask to apply to that page
	int mask = 0x1F << (ball_y%8);
	int i = page*128 + x;			//this calculates the x location of the ball
	char k;
	if (ball_y > 64 - 5)	//if the ball hits the bottom edge
	{
		y_vel = -1;		// set the y_vel upwards
		TMR3RLL = 0xA8;	//these values are chosen to create a certain sound with the DAC
		TMR3RLH = 0xF9;
		REF0CN = 3;	// turn on voltage reference
		DAC0CN = 0x88; 	// update on timer 2, left justified
		TMR3CN = 0x06;	//tier 3 autoreload
		duration = 50;  //run the DAC through 50 cycles
	}
	if (ball_y < 1)		//if the ball hits the top edge
	{
		y_vel = 1;		//set the y_vel downwards
		TMR3RLL = 0xA8;	//these values are choosen to create the sound for hitting an edge
		TMR3RLH = 0xF9;
		REF0CN = 3;	// turn on voltage reference
		DAC0CN = 0x88; 	// update on timer 2, left justified
		TMR3CN = 0x06;	//timer 3 autoreload
		duration = 50;  //run the DAC through 50 cycles
	}
	if (ball_x > 128 - 5)
		if (ball_y + 5 > left_paddle && ball_y < left_paddle + lpad_size*4 + 8)	//if the ball has hit the right paddle
			{							//lpad_size*4 + 8 includes the entire width of the paddle
				x_vel = -1;
				TMR3RLL = 0x60;	//these values are choosen to make a specific sound for the ball hitting a paddle
				TMR3RLH = 0xF9;
				REF0CN = 3;	// turn on voltage reference
				DAC0CN = 0x88; 	// update on timer 2, left justified
				TMR3CN = 0x06;	//timer 3 autoreload
				duration = 50;
				if (ball_y + 5 > left_paddle && ball_y < (left_paddle + (lpad_size*4 + 8)/4))// these 4 if statements
				{						//determine what portion of the paddle the ball hit byt dividing the paddle into 4 chunks
					angle = 0;			//depending on the location hit a different value is assigned to angle and y_vel
					y_vel = -1;			//these values create 4 different angles
				}						//the numbers in the if statements divide the paddle bounds into 4
				if (ball_y > left_paddle + (lpad_size*4 + 8)/4 && ball_y < (left_paddle + (lpad_size*4 + 8)/4)*2)
				{
					angle = 1;
					y_vel = -1;
				}
				if (ball_y > (left_paddle + (lpad_size*4 + 8)/4)*2 && ball_y < (left_paddle + (lpad_size*4 + 8)/4)*3)
				{
					angle = 1;
					y_vel = 1;
				}
				if (ball_y > (left_paddle + (lpad_size*4 + 8)/4)*3 && ball_y < (left_paddle + (lpad_size*4 + 8)))
				{
					angle = 0;
					y_vel = 1;
				}
			}
	if (ball_x < 1)
		if (ball_y + 5 > right_paddle && ball_y < right_paddle + rpad_size*4 + 8)	// this is the same 
			{																//as above only for the other paddle
				x_vel = 1;
				TMR3RLL = 0x60;
				TMR3RLH = 0xF9;
				REF0CN = 3;	// turn on voltage reference
				DAC0CN = 0x88; 	// update on timer 2, left justified
				TMR3CN = 0x06; //timer 3 autoreload
				duration = 50;
				if (ball_y + 5 > right_paddle && ball_y < (right_paddle + (rpad_size*4 + 8)/4))
				{
					angle = 0;
					y_vel = -1;
				}
				if (ball_y > right_paddle + (rpad_size*4 + 8)/4 && ball_y < (right_paddle + (rpad_size*4 + 8)/4)*2)
				{
					angle = 1;
					y_vel = -1;
				}
				if (ball_y > (right_paddle + (rpad_size*4 + 8)/4)*2 && ball_y < (right_paddle + (rpad_size*4 + 8)/4)*3)
				{
					angle = 1;
					y_vel = 1;
				}
				if (ball_y > (right_paddle + (rpad_size*4 + 8)/4)*3 && ball_y < (right_paddle + (rpad_size*4 + 8)))
				{
					angle = 0;
					y_vel = 1;
				}				
			}
	for (k = 0; k < 5; k++, i++, x++)		//this function draws the ball on the screen
	{
		if (x >= 0 && x < 128)
		{
			screen[i]|= mask;				//prints the ball on the first page it is on
			screen[i + 128]|= mask >> 8;	//prints the ball on the second page it is on if it 
		}									//is on more than one page
	}
}


//interrupt for the ADC for sampling the potentiometers
//external variable used: ADCdata, ADC0H, ADC0CN, AMX0Sl, ADC0CF, ADC0L, tacc, t_count, M, rawpot, rawpot2,
// pacc, p_count

void pots(void) interrupt 15
{
	ADCdata = ADC0H *256;	//moving the ADC0H into the top byte of ADCdata
	ADC0CN = 0x8C;			//changing ADC input to the other pot
	if (AMX0SL == 1)	//if it was pot1 now its pot 2
	{
		ADC0CF = 0x40;	//sets conversion rate
		AMX0SL = 0;		//sets the multipler to 1
		ADCdata = ADCdata + ADC0L;	//moves ADC0L into the low byte of ADCdata
		tacc = tacc + ADCdata;		//tallies up ADCdata for averaging
		t_count++;					//keeps track of how many samples taken
		if (t_count != M)			//if the # of samples is less than M (256) keep taking samples
		{
			return;
		}
		rawpot2 = tacc/M;			//if 256 samples is reached divide the total by 256 to get an average
		tacc = 0;					//reset both the tally and the count
		t_count = 0;
	}
	else if (AMX0SL == 0)	// if it was pot 2 now its pot 1
	{
		ADC0CF = 0x40;		//this code is identical to the code above but instead calculates pot2's values
		AMX0SL = 1;
		ADCdata = ADCdata + ADC0L;
		pacc = pacc + ADCdata;
		p_count++;
		if (p_count != M)
		{
			return;
		}
		rawpot = pacc/M;
		pacc = 0;

		p_count = 0;
	}
	return;
}


//interrupt for timer 3 to run the DAC to generate sound
//external variables used: TMR3CN, DAC0H,  duration, phase, sine[]
void timer3(void) interrupt 14
{
	TMR3CN = 0x06;			//clear timer 3 overflow
	DAC0H = sine[phase];	//load the value from the sine array into the DAC value
	if ( phase < sizeof(sine)-1 )	// if mid-cycle
	{				// complete it
		phase++;
	}
	else if ( duration > 0 )	// if more cycles left to go
	{				// start a new cycle
		phase = 0;
		duration--;
	}
	if (duration == 0)
		TMR3CN = 0x02;	//disable timer 3
}