#include <C8051F020.h>

code unsigned char sine[] = { 176, 217, 244, 254, 244, 217, 176, 128, 80, 39, 12, 2, 12, 39, 80, 128 };

unsigned char phase = sizeof(sine)-1;	// current point in sine to output

unsigned int duration = 0;		// number of cycles left to output
double decay = 0.01;

void timer2(void) interrupt 5
{
	TF2 = 0;
	//DAC0H = sine[phase];
	DAC0H = sine[phase] - sine[phase]*decay;
	if ( phase < sizeof(sine)-1 )	// if mid-cycle
	{				// complete it
		phase++;
	}
	else if ( duration > 0 )	// if more cycles left to go
	{				// start a new cycle
		phase = 0;
		duration--;
		decay = decay + .0012;
	}
}

sbit BUTTON = P3^7;
sbit BUTTON2 = P1^1;

void main(void)
{
 	WDTCN=0x0DE; 	// disable watchdog
	WDTCN=0x0AD;
	XBR2=0x40;	// enable port output
	OSCXCN=0x67;	// turn on external crystal
	TMOD=0x20;	// wait 1ms using T1 mode 2
	TH1=256-167;	// 2MHz clock, 167 counts = 1ms
	TR1 = 1;
	while ( TF1 == 0 ) { }
	while ( (OSCXCN & 0x80) == 0 ) { }
	OSCICN=0x8;	// engage! Now using 22.1184MHz
	CKCON = 0x20;
	T2CON = 4;	// timer 2, auto reload
	RCAP2H = 0xF9;
	RCAP2L = 0x40;	// set up for 800Hz initially
	REF0CN = 3;	// turn on voltage reference
	DAC0CN = 0x9C; 	// update on timer 2, left justified
	IE = 0xA0;	// enable timer 2 only

	for ( ; ; )
	{
		while ( duration || (BUTTON && BUTTON2)) { }	// wait till finished and button pressed
		if (BUTTON)
		{
			RCAP2H = 0xF9;
			RCAP2L = 0x40;				// set up for 800Hz
	   		duration = 800;				// one second
			decay = 0.01;
			while (duration){}
			RCAP2H = 0xF7;
			RCAP2L = 0x84;				// set up for 635 Hz
			duration = 635;				// one second
			decay = 0.01;
		}
		else if(BUTTON2)
		{
			RCAP2H = 0xF9;
			RCAP2L = 0xFF;				// set up for 800Hz
		   	duration = 800;				// one second
			decay = 0.01;
			while (duration){}
			RCAP2H = 0xF8;
			RCAP2L = 0x84;				// set up for 635 Hz
			duration = 635;				// one second
			decay = 0.01;
		}
	}
}