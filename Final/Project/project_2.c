#include<reg51.h>
#include<intrins.h>
#include <stdlib.h>

sbit SRCLK=P3^6;
sbit RCLK=P3^5;
sbit SER=P3^4;

unsigned char dcount;
typedef unsigned char u8;
bit flag = 0;
u8 revData;
u8 arr[8] = {0,0,0,0,0,0,0,0};
u8 header[7] = {0x54, 0x69, 0x65, 0x6E, 0x53, 0x79}; // 'T', 'i', 'e', 'n', 'S', 'y'
u8 size_data = 0;	// So luong byte du lieu sau header
u8 fps = 0;				//frame per second
u8 dem = 0;

sbit led1= P2^0;
sbit led2 = P2^1;

u8 code TAB[8]  = {0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};
#define COMMONPORTS P0

void serial_init(void){
	/*fcrystal = 11.0592MHz*/
	/*config timer 1*/
	TMOD = 0x20;
	TH1 = 0xFD;
	SCON = 0x50;
	TR1 = 1;
	EA = 1;
	ES = 1;
}


void ISR_timer0() interrupt 1
{
    TH0 = 0xFC;
    TL0 = 0x18;
    dcount++;
}

void delayMS(unsigned int time)
{
	int i, j;
	for(i = 0; i < time; i++)
		for(j = 0; j < 123; j++) {}
}

void serial_Send(u8 x) {
	SBUF = x;
	while(TI == 0); //wait until transmiting completed
	TI = 0;
}
void serial_ISR(void) interrupt 4
{
	if(RI == 1) {
		revData = SBUF;
		RI = 0;
		flag = 1;
		arr[dem] = revData;
		dem++;
		if(dem == 8) 
		{
			dem = 0;
		}
	}
	//if (TI == 1) ... only use when need interrupT for sending dataa
}

u8 check_header(void)
{
	u8 i;
	u8 check = 1;
	for(i = 0; i < 6; i++)
	{
		if(arr[i] != header[i])
		{
			check = 0;
			return check;
		}
	}
	if(check == 1){
		fps = arr[6];
		size_data = arr[7];
	}
	return check;
}

void Hc595SendByte(u8 dat)
{
    u8 a;
    SRCLK=0;
    RCLK=0;
    for(a=0;a<8;a++)
    {
        SER=dat>>7;
        dat<<=1;
        SRCLK=1;
        SRCLK=0;    
    }
    RCLK=1;
    RCLK=0;
}
void display(void)
{
	u8 tab, i;
	for(i = 0; i < 4; i++)
	{
		for(tab=0;tab<8;tab++)
		{      
			Hc595SendByte(0x00); 
			COMMONPORTS = TAB[tab];             
			Hc595SendByte(arr[tab]);    
			delayMS(6);     
		}
	}
}

void main()
{   
	
	led1 = 0;
	serial_init();
  while(1)
  {   
		if(check_header())
		{
			display();
		}
  }   
}
