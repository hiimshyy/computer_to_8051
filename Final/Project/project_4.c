#include<reg51.h>
#include<intrins.h>
#define COMMONPORTS P0

sbit SRCLK = P3^6;
sbit SER = P3^4;
sbit RCLK = P3^5;
bit state = 0;  //state = 0 de nhan du lieu va kiem tra. state = 1 de ghi du lieu vao bo nho
bit start_stop_state = 0;	//0 : START, 1: STOP
typedef unsigned char u8;
typedef unsigned int u16;
u8 revData;
u8 arr[8];
u16 size_data = 0;
u8 header[8] = {0x2A, 0x2A, 0x2A, 0, 0, 0, 0, 0};
//header[3] & header[4] to save size_data
//header[5] to save fps of video
u8 frame_array[8][8];
u8 code TAB[8]  = {0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};
u8 i, number_of_frame, k = 0;

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
void reset_frame_array(void)
{
	u8 i, j;
	for(i = 0; i < 8; i++)
		for(j = 0; j < 8; j++)
			frame_array[i][j] = 0x00;
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
	Hc595SendByte(0x00);
	Hc595SendByte(0xFF);
	delayMS(200);	
	Hc595SendByte(0x00);
}

bit check_header()
{
	u8 i;
	bit check = 1;
	for(i = 0; i < 3; i++)
	{
		if(arr[i] != header[i])
		{
			check = 0;
		}
	}
	if(check)
	{
		header[3] = arr[3];
		header[4] = arr[4];
		header[5] = arr[5];
		size_data = (header[4] << 8) | header[3];
	}
	return check;
}

void serial_ISR(void) interrupt 4
{
	if(RI == 1) {
		if(state == 0){
			revData = SBUF;
			RI = 0;
			arr[i] = revData;
			i++;
			if(i == 8){
				i = 0;
				if(check_header())
				{
					state = 1;
					reset_frame_array();
				}
			}
		}else
		{
			revData = SBUF;
			RI = 0;
			frame_array[number_of_frame][k] = revData;
			k++;
			if(k == 8)
			{
				k = 0;
				number_of_frame++;
			}
			if(number_of_frame == size_data/8)
			{
				number_of_frame = 0;
				state = 0;
			}
		}
	}
}

void display_to_matrix_led(void)
{
	u8 tab, i, k, number_of_frame;
	number_of_frame = size_data/8;
	for(i = 0; i < number_of_frame; i++)
	{
		k = 0;
		while(k < header[5])
		{
			for(tab=0;tab<8;tab++)
			{
				Hc595SendByte(0x00); 
				COMMONPORTS = TAB[tab];             
				Hc595SendByte(frame_array[i][tab]);    
				delayMS(2);     
			}
		k++;
		}
		if(i >= number_of_frame - 1) i = -1;
	}
}

void main(void)
{
	COMMONPORTS = 0x00;
	serial_init();
	test();
	while(1)
	{
		display_to_matrix_led();
	}
}