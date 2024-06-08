#include<reg51.h>
#include<intrins.h>
#define COMMONPORTS P0

sbit SRCLK = P3^6;
sbit SER = P3^4;
sbit RCLK = P3^5;

typedef unsigned char u8;
typedef unsigned int u16;

bit flag;
u8 revData;
u8 arr[8][8];
u8 size_data_high = 0;	// So luong byte du lieu sau header
u8 size_data_low = 0;	// So luong byte du lieu sau header
u16 size_data = 0;
u8 end_data;
u8 fps = 0;				//frame per second
u8 i = 0;
u8 ghc595_buf[8]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x00};

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

void Hc595SendByte(unsigned char dat)
{
    unsigned char a;
    SRCLK=0;
    RCLK=0;
    for(a=0;a<8;a++)
    {
        SER=dat>>7;
        dat<<=1;

        SRCLK=1;
        _nop_();
        _nop_();
        SRCLK=0;    
    }

    RCLK=1;
    _nop_();
    _nop_();
    RCLK=0;
}

void delayMS(unsigned int time)
{
	int i, j;
	for(i = 0; i < time; i++)
		for(j = 0; j < 123; j++) {}
}
void test(void)
{
	u8 i;
	for(i=0;i<8;i++)
	{	
		Hc595SendByte(0x00);
		Hc595SendByte(ghc595_buf[i]);
		delayMS(100);	
	}
}
/*
void serial_Send(u8 x) {
	SBUF = x;
	while(TI == 0); //wait until transmiting completed
	TI = 0;
}
void serial_Send_String(u8 x[]) {
    u8 i = 0;
    // Loop until null character is encountered
    while (x[i] != '\0') {
        serial_Send(x[i]);
        i++;
    }
}
*/
void create_array()
{
	u16 i, j;
	u8 number_of_frame;
	size_data = (u16)((size_data_high << 8) | size_data_low);
	number_of_frame = size_data/8;
	for(i = 0; i < number_of_frame; i++)
	{
		for(j = 0; j < 8; j++)
		if(flag == 1)
		{
			flag = 0;
			arr[i][j] = revData;
			while(RI == 0);
		}
	}
	end_data = revData;
	flag = 0;		
}

void display_to_matrix_led(void)
{
	u8 tab, i, k, number_of_frame;
	number_of_frame = size_data/8;
	for(i = 0; i < number_of_frame; i++)
	{
		k = 0;
		while(k < fps)
		{
		for(tab=0;tab<8;tab++)
		{
			Hc595SendByte(0x00); 
			COMMONPORTS = TAB[tab];             
			Hc595SendByte(arr[i][tab]);    
			delayMS(2);     
		}
		k++;
		}
		if(i >= number_of_frame - 1) i = -1;
	}
	
}

void read_size_and_fps(void)
{
	if(flag == 1)
	{
		flag = 0;
		size_data_low = revData;
		while(RI == 0);
		if(flag == 1)
		{
			flag = 0;
			size_data_high = revData;
			
			while(RI == 0);
			if(flag == 1)
			{
				flag = 0;
				fps = revData;
				while(RI == 0);
			}
		}
	}
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
u8 is_header()
{
	if(flag == 1)
	{
		if(revData == '*')
		{
			flag = 0;
			while(RI == 0);
			if(revData == '*')
			{
				flag = 0;
				while(RI == 0);
				return 1;
			}
		}
	}
	return 0;
}

void main(void)
{
	COMMONPORTS = 0x00;
	serial_init();
	test();
	COMMONPORTS = 0x00;
	while(1)
	{
		if(is_header())
		{
			read_size_and_fps();
			create_array();
			if(end_data == '*')
			{
				display_to_matrix_led();
			}
		}
	}
}