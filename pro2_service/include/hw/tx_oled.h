#ifndef __TX_OLED_H
#define __TX_OLED_H

//int tx_oled_init();	//oled设备初始化
//
//int tx_oled_exit(); //oled设备关闭
//
//int tx_oled_cls();  //清屏
//
//int tx_oled_on();	//turn on
//
//int tx_oled_off();  //turn off
//
//int tx_oled_drawbmp(int x,int y, u8 *bmp , int size); //在(x,y)画bmp的二值图,size表示点阵大小
//
//int tx_oled_drawstr(int x,int y, u8 *str , int size); //在(x,y)显示字符串str,size表示字符长度
//
//int tx_oled_drawicon(int x, int y,int w, int h ,u8 *bmp ,int size);
#if 0
int tx_oled_init();	//oled设备初始化

int tx_oled_exit(); //oled设备关闭

int tx_oled_cls();  //清屏

int tx_oled_on();	//turn on

int tx_oled_off();  //turn off

int tx_oled_char_icon(int x,int y, int w, int h, u8 *ip);  //在坐标(x,y)显示字符的bmp，支持高宽6x8 8x11

int tx_oled_reset(); //OLED 硬件复位

int tx_oled_fill(int x, int y, int w , int h, int val); //val=0x00清屏  val=0xFF点亮该区域 

int tx_oled_drawstr(int x,int y, int w, int h ,u8 *str, int size); //在(x,y)显示字符串str,size表示字符长度

int tx_oled_drawicon(int x, int y,int w, int h ,u8 *bmp, int size);

int tx_olde_draw_icon_ex(int id);
#endif
int tx_oled_init();	//oled�豸��ʼ��

int tx_oled_exit(); //oled�豸�ر�

int tx_oled_cls();  //����

int tx_oled_on();	//turn on

int tx_oled_off();  //turn off

int tx_oled_reset();

int tx_oled_fill(int x, int y, int w , int h, int val); //val=0x00 or 0xFF

int tx_oled_char_icon(int x , int  y, int w ,int h , u8 *ip);

int tx_oled_draw_icon_ex(int id);//


///////////////////////////////////key_evet ////////////////////////

#define MAX_INPUT_EV_NUM	3

typedef struct __key_event
{
    int type;
    int value;
    int code;
}key_event;

int tx_ket_init();

int tx_key_read(key_event *e);

int tx_key_exit();

#endif