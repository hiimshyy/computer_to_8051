#include<regx52.h>
#include <stdlib.h>
#include<intrins.h>

typedef unsigned char u8;
bit flag = 0;
u8 revData;

sbit SRCLK=P3^6;
sbit RCLK=P3^5;
sbit SER=P3^4;

sbit led1= P2^0;
sbit led2 = P2^1;
sbit btn1 = P2^2;
u8 dcount;
//u8 arr[8] = {0,0,0,0,0,0,0,0};
u8 arr[8] = {0x00, 0x00, 0x40, 0x40, 0x7E, 0x40, 0x40, 0x00};
#define COMMONPORTS P0

u8 code TAB[8]  = {0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};
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

void delayMS(unsigned int t)
{
    TMOD = 0x01;
    TH0 = 0xFC;
    TL0 = 0x18;
    IE = 0x82;
    dcount = 0;
    TR0 = 1;
    while(dcount < t){}
    TR0 = 0;
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
	}
	//if (TI == 1) ... only use when need interrupT for sending dataa
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

        SRCLK=1; // 

        SRCLK=0;    
    }

    RCLK=1;

    RCLK=0;
}
void display()
{
	u8 tab;
	for(tab=0;tab<8;tab++)
  {      
		Hc595SendByte(0x00); 
    COMMONPORTS = TAB[tab];             
    Hc595SendByte(arr[tab]);    
    delayMS(6);     
  }
}
void handleData(u8 SIZE)
{
	u8 i;
	//u8 arr[8] = {0,0,0,0,0,0,0,0};
	for(i = 0; i < SIZE; i++)
	{
		arr[i] = revData;
		while(RI == 0);
	}
	if(arr[2] == 0x40){	serial_Send('H');}
}
void loop()
{
	u8 Size = 0;
	bit check_header = 1;
	u8 i = 0;
	//u8 header_code[8] = {0x54, 0x52, 0x49, 0x45, 0x55, 0x4d, 0x45};
	u8 header_code[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	if(flag == 1)
	{
		for(i = 0; i < 7; i++)
		{
			if(flag == 1)
			{
				if(revData != header_code[i])
				{
					check_header = 0;
					flag = 0;
					break;
				}
				flag = 0;
				while(RI == 0);
			}
		}
		Size = revData;
		while(RI == 0);
	}
	
	if(check_header)
	{
		led1 = ~led1;
		handleData(Size);
	}
}

void main(void)
{
	unsigned char tab;
  unsigned int  i;
	serial_init();
    while(1)
    {   
        for(i = 0; i<30; i++ )   
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
}