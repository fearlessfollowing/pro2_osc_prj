#include <common/include_common.h>
#include <hw/oled_module.h>
#include <util/font_ascii.h>
#include <util/icon_ascii.h>
#include <hw/ins_i2c.h>
#include <hw/ins_gpio.h>


#define OLED_I2C_BUS		2
#define OLED_RESET_GPIO		259	// PRO1 -> 174; PRO2 ->259


#define COL_MAX 	(128)
#define ROW_MAX 	(64)
#define PAGE_MAX 	(8)
#define MAX_BUF 	(1024)

// REG INDEX
#define SSD_SET_COL0_START_ADDR     0x00    // Set  Lower Column Start Address for Page Addressing Mode (00h~0Fh)
#define SSD_SET_COL1_START_ADDR     0x10    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
#define SSD_SET_PAGE_START_ADDR     0xB0    // Set GDDRAM Page Start Address(PAGE0~PAGE4) for Page Addressing Mode (B0h~B4h)

#define SSD_SET_DISPLAY_LINE        0x40    // Set Display Start Line (40h~66h)
#define SSD_SET_CONTRAST            0x81    // Set Contrast Control(81h)

#define SSD_SET_SEGMENT_NORMAL      0xA0    // Column address 0   is mapped to SEG0 (A0h) (RESET)
#define SSD_SET_SEGMENT_REVERSE     0xA1    // Column address 131 is mapped to SEG0 (A1h)

#define SSD_SET_DISPLAY_RAM         0xA4    // Resume to RAM content display, Output follows RAM content (A4h) (RESET)
#define SSD_SET_DISPLAY_ALL_ON      0xA5    // Entire display ON, Output ignores RAM content (A5h)

#define SSD_SET_NORMAL_DISPLAY      0xA6    // Set Normal display (A6h) (RESET)
#define SSD_SET_INVERSE_DISPLAY     0xA7    // Set Inverse display (A7h)

#define SSD_SET_MULTIPLEX_RATIO     0xA8    // Set Multiplex Ratio (A8h)

#define SSD_SET_DISPLAY_OFF         0xAE    // Set Display OFF in sleep mode (AEh) (RESET)
#define SSD_SET_DISPLAY_ON          0xAF    // Set Display ON in normal mode (AFh)

#define SSD_SET_COM_SCAN_NORMAL     0xC0    // Set COM Output Scan Direction in normal mode, Scan from COM0 to COM[N �C1] (C0h) (RESET)
#define SSD_SET_COM_SCAN_INVERSE    0xC8    // Set COM Output Scan Direction in remapped mode, Scan from COM[N-1] to COM0 (C8h)

#define SSD_SET_DISPLAY_OFFSET      0xD3    // Set Display Offset (D3h) Set Vertical shift by COM from 0d~63d, The value is reset to 00h after RESET.
#define SSD_SET_DISPLAY_CLOCK       0xD5    // Set Display Clock Divide Ratio/ Oscillator Frequency (D5h)
#define SSD_SET_PRECHARGE_PERIOD    0xD9    // Set Pre-charge Period (D9h)
#define SSD_SET_SEG_PINS_CONFIG     0xDA    // Set SEG Pins Hardware Configuration (DAh)
#define SSD_SET_VCOM_LEVEL          0xDB    // Set VCOM Deselect Level (DBh)
#define SSD_SET_NOP                 0xE3    // NOP, No Operation Command (E3h)

#define SSD_SET_ADDRESS_MODE        0x20    // Set Memory Addressing Mode (20h)
#define SSD_SET_COLUMN_ADDRESS      0x21    // Setup column start and end address only for Horizontal or Vertical Addressing Mode
#define SSD_SET_PAGE_ADDRESS        0x22    // Setup page   start and end address only for Horizontal or Vertical Addressing Mode

#define SSD_SET_HOR_R_SCROLL        0x26    // Right Horizontal Scroll (Horizontal scroll by 1 column)
#define SSD_SET_HOR_L_SCROLL        0x27    // Left  Horizontal Scroll (Horizontal scroll by 1 column)
#define SSD_SET_VER_R_SCROLL        0x29    // Vertical and Right Horizontal Scroll (Horizontal scroll by 1 column)
#define SSD_SET_VER_L_SCROLL        0x2A    // Vertical and Left  Horizontal Scroll (Horizontal scroll by 1 column)
#define SSD_SET_VER_SCROLL_AREA     0xA3    // Set Vertical Scroll Area

#define SSD_SET_DEACTIVATE_SCROLL   0x2E    // Stop scrolling that is configured by command 26h/27h/29h/2Ah.
#define SSD_SET_ACTIVATE_SCROLL     0x2F    // Start scrolling that is configured by the scrolling setup commands 26h/27h/29h/2Ah

#define SSD_SET_CHARGE_PUMP         0x8D

#define SSD_CMD_ADDRESS 			0x00    // cmd reg address
#define SSD_DAT_ADDRESS 			0x40    // data reg address

// 0x10 -- new  0x14 -- old
#define CHARGE_PUMP_VAL 			0x10	//0x14//


#define TAG 	("oled_module")
#define OLD_OLED

//#define HORIZONAL_ADDRESS_MODE
//#define USE_U64

enum {
	FILL_EMPTY,
	FILL_FULL,
};

enum {
    FRAME_5 = 0,
    FRAME_64,
    FRAME_128,
    FRAME_256,
    FRAME_3,
    FRAME_4,
    FRAME_25,
    FRAME_2,
};


typedef struct _page_info_ {
    u8 delta_y;
    u8 page_start;
    //page num that dat occupied
    u8 dat_page_num;
	
    //page num that need upate(same as dat_page_num or dat_page_num + 1)
    u8 page_num;
} PAGE_INFO;

static const u8 PAGE_H = 8;
static const u8 char_h = 16;
const u8 fill_dat[] = {0x00, 0xff};

#define SDCARD_TEST_SUC		"/data/etc/.pro_old"

#if 0
bool check_old_pro()
{
    bool ret = false;
    char buf[1024];

    snprintf(buf,sizeof(buf),"%s", SDCARD_TEST_SUC);

    if (check_path_exist(buf)) {
        ret = true;
    }

    Log.d(TAG, "check_old_pro (%s %d)", buf, ret);
    return ret;
}
#endif

oled_module::oled_module()
{
    init();
}

oled_module::~oled_module()
{
    deinit();
}

void oled_module::init()
{
    ucBuf = (u8 *)malloc(MAX_BUF);
    CHECK_NE(ucBuf, nullptr);
    ucBufLast = (u8 *)malloc(MAX_BUF);
    CHECK_NE(ucBufLast, nullptr);
    memset(ucBuf, 0, MAX_BUF);
    memset(ucBufLast, 0, MAX_BUF);

	/* OLED位于I2C总线2,地址为0x3c */
    pstI2C_OLED = sp<ins_i2c>(new ins_i2c(OLED_I2C_BUS, 0x3c));
    CHECK_NE(pstI2C_OLED, nullptr);

    mPageInfo = sp<PAGE_INFO>(new PAGE_INFO());
    CHECK_NE(mPageInfo, nullptr);

    ssd1306_init();

//    CHECK_EQ(ICON_MAX, sizeof(oled_icons)/sizeof(oled_icons[0]));
}

void oled_module::deinit()
{
    ssd1306_set_off();
	
    //disable charge Pump
//    SSD_Set_Charge_Pump(0x10);
    ssd1306_reset();

    if (ucBuf) {
        free(ucBuf);
        ucBuf = nullptr;
    }

    if (ucBufLast) {
        free(ucBufLast);
        ucBufLast = nullptr;
    }
}

void oled_module::ssd1306_reset()
{   
    if (gpio_is_requested(OLED_RESET_GPIO) != 1) {
        gpio_request(OLED_RESET_GPIO);
    }

    gpio_direction_output(OLED_RESET_GPIO, 0);
    msg_util::sleep_ms(500);
    gpio_direction_output(OLED_RESET_GPIO, 1);
}


// addr:    ( 0 ~ 127 ) (RESET = 0)
void oled_module::SSD_Set_Start_Column(u8 addr)
{
    ssd1306_write_cmd(SSD_SET_COL0_START_ADDR | (addr & 0x0F)); // Set  Lower Column Start Address
    addr >>= 4;
    ssd1306_write_cmd(SSD_SET_COL1_START_ADDR | (addr & 0x0F)); // Set Higher Column Start Address
}


/*----------------------------------------------------------------------------*/
// SSD1306
// addr:    ( 0 ~ 7 ) (RESET = 0)
void oled_module::SSD_Set_Start_Page(u8 addr)
{
    ssd1306_write_cmd(SSD_SET_PAGE_START_ADDR + addr); // Set Page Start Address
}


/*----------------------------------------------------------------------------*/
// SSD1306
// start:   ( 0 ~ 127 ) (RESET = 0)
// end:     ( 0 ~ 127 ) (RESET = 127)
void oled_module::SSD_Set_Column_Address(u8 start, u8 end)
{
    ssd1306_write_cmd(SSD_SET_COLUMN_ADDRESS); // Setup Column start and end address
    ssd1306_write_cmd(start);
    ssd1306_write_cmd(end);
}

/*----------------------------------------------------------------------------*/
// SSD1306
// start:   ( 0 ~ 7 ) (RESET = 0)
// end:     ( 0 ~ 7 ) (RESET = 7)
void oled_module::SSD_Set_Page_Address(u8 start, u8 end)
{
    ssd1306_write_cmd(SSD_SET_PAGE_ADDRESS); // Setup page start and end address
    ssd1306_write_cmd(start);
    ssd1306_write_cmd(end);
}
/*----------------------------------------------------------------------------*/
// SSD1306
// attr:    1 = Display ON in normal mode
//          0 = Display OFF in sleep mode (RESET)
void oled_module::SSD_Set_Display_ON_OFF(u8 attr)
{
    if (attr) { // Set Display ON
        ssd1306_write_cmd(SSD_SET_DISPLAY_ON);
    } else {    // Set Display OFF
        ssd1306_write_cmd(SSD_SET_DISPLAY_OFF);
    }
}


/*----------------------------------------------------------------------------*/
// SSD1306
// line:    ( 0 ~ 63 ) (RESET = 0)
void oled_module::SSD_Set_Start_Line(u8 line)
{
    ssd1306_write_cmd(SSD_SET_DISPLAY_LINE + line); // Set Display Start Line
}


/*----------------------------------------------------------------------------*/
// SSD1306
// mode:    0 = Horizontal Addressing Mode
//          1 = Vertical Addressing Mode
//          2 = Page Addressing Mode (RESET)
void oled_module::SSD_Set_Address_Mode(u8 mode)
{
    ssd1306_write_cmd(SSD_SET_ADDRESS_MODE); // Set Memory Addressing Mode
    ssd1306_write_cmd(mode);
}

/*----------------------------------------------------------------------------*/
// SSD1306
void oled_module::SSD_Set_Contrast(u8 dat)
{
    ssd1306_write_cmd(SSD_SET_CONTRAST); // Set Contrast Control
    ssd1306_write_cmd(dat);
}

/*----------------------------------------------------------------------------*/
// SSD1306
// attr:    1 = Column Address 0 Mapped to SEG127   (Reverse)
//          0 = Column Address 0 Mapped to SEG0     (RESET = Normal)
void oled_module::SSD_Set_Segment_Remap(u8 attr)
{
    if (attr)
    {
        ssd1306_write_cmd(SSD_SET_SEGMENT_REVERSE);
    } // Set Segment Reverse
    else
    {
        ssd1306_write_cmd(SSD_SET_SEGMENT_NORMAL);
    } // Set Segment Normal
}

/*----------------------------------------------------------------------------*/
// SSD1306
// attr:    1 = Scan from COM[N-1] to COM0
//          0 = Scan from COM0 to COM[N-1] (RESET)
void oled_module::SSD_Set_Common_Remap(u8 attr)
{
    if (attr)
    {
        ssd1306_write_cmd(SSD_SET_COM_SCAN_INVERSE);
    } // Set Common Re-Map
    else
    {
        ssd1306_write_cmd(SSD_SET_COM_SCAN_NORMAL);
    } // Normal
}

/*----------------------------------------------------------------------------*/
// SSD1306
// dat: 0x3F for 64
void oled_module::SSD_Set_Multiplex_Ratio(u8 dat)
{
    ssd1306_write_cmd(SSD_SET_MULTIPLEX_RATIO); // Set Multiplex Ratio
    ssd1306_write_cmd(dat);
}

/*----------------------------------------------------------------------------*/
// SSD1306
// attr:
//
void oled_module::SSD_Set_Entire_Display(u8 attr)
{
    if (attr)
    {
        ssd1306_write_cmd(SSD_SET_DISPLAY_ALL_ON);
    } // Display All ON
    else
    {
        ssd1306_write_cmd(SSD_SET_DISPLAY_RAM);
    } // Display RAM
}

/*----------------------------------------------------------------------------*/
// SSD1306
// attr:
//
void oled_module::SSD_Set_Inverse_Display(u8 attr)
{
    if (attr)
    {
        ssd1306_write_cmd(SSD_SET_INVERSE_DISPLAY);
    } // Inverse Display
    else
    {
        ssd1306_write_cmd(SSD_SET_NORMAL_DISPLAY);
    } // Normal  Display
}

/*----------------------------------------------------------------------------*/
// SSD1306
// dat:     ( 0 ~ 63 ) (RESET = 0)
void oled_module::SSD_Set_Display_Offset(u8 dat)
{
    ssd1306_write_cmd(SSD_SET_DISPLAY_OFFSET); // Set Display Offset
    ssd1306_write_cmd(dat);
}

/*----------------------------------------------------------------------------*/
// SSD1306
// dat[7:4]:    ( 0000b ~ 1111b )Set the Oscillator Frequency Fosc = dat[7:4]. (RESET = 1100b)
// dat[3:0]:    ( 0000b ~ 1111b )Display Clock Divider Ratio D = dat[3:0] + 1. (RESET = 0001b)
// note:        DCLK = Fosc / D
void oled_module::SSD_Set_Display_Clock(u8 dat)
{
    ssd1306_write_cmd(SSD_SET_DISPLAY_CLOCK); // Set Display Clock Divide Ratio / Oscillator Frequency
    ssd1306_write_cmd(dat);
}

/*----------------------------------------------------------------------------*/
// SSD1306
// dat[7:4]:    ( 0001b ~ 1111b )Pre-Charge period in 1~15 Display Clocks (RESET = 2)
// dat[3:0]:    ( 0001b ~ 1111b )Discharge  period in 1~15 Display Clocks (RESET = 2)
void oled_module::SSD_Set_Precharge_Period(u8 dat)
{
    ssd1306_write_cmd(SSD_SET_PRECHARGE_PERIOD); // Set Pre-Charge Period
    ssd1306_write_cmd(dat);
}

/*----------------------------------------------------------------------------*/
// SSD1306
// alternative: 0 = Sequential  SEG pin configuration
//              1 = Alternative SEG pin configuration (RESET)
// left_right:  0 = Disable SEG Left/Right remap (RESET)
//              1 = Enable  SEG Left/Right remap
void oled_module::SSD_Set_Segment_Config(u8 left_right, u8 alternative)
{
    ssd1306_write_cmd(SSD_SET_SEG_PINS_CONFIG); // Set SEG Pins Hardware Configuration
    ssd1306_write_cmd((left_right << 5) | (alternative << 4) | 0x02);
}

/*----------------------------------------------------------------------------*/
// SSD1306
// level:   0x00 = 0.65*VCC
//          0x20 = 0.77*VCC (RESET)
//          0x30 = 0.83*VCC
void oled_module::SSD_Set_VCOM_Level(u8 level)
{
    ssd1306_write_cmd(SSD_SET_VCOM_LEVEL); // Set VCOM Deselect Level
    ssd1306_write_cmd(level);
}

void oled_module::SSD_Set_Charge_Pump(u8 val)
{
    ssd1306_write_cmd(SSD_SET_CHARGE_PUMP); // Set VCOM Deselect Level
   // if (check_old_pro())
    //{
        ssd1306_write_cmd(0x14);
   // }
   // else
   // {
        Log.d(TAG, "2charge pump val 0x%x", val);
   //     ssd1306_write_cmd(val);
   // }
}

/*----------------------------------------------------------------------------*/
// SSD1306
void oled_module::SSD_Set_Deactivate_Scroll(void)
{
	ssd1306_write_cmd(SSD_SET_DEACTIVATE_SCROLL); // Deactivate Scrolling
}


/*----------------------------------------------------------------------------*/
// SSD1306
void oled_module::SSD_Set_Nop(void)
{
    ssd1306_write_cmd(SSD_SET_NOP); // Command for No Operation
}


/*----------------------------------------------------------------------------*/
// SSD1306
// LR:      0 = Right Horizontal Scroll
//          1 = Left  Horizontal Scroll
// start_page:  ( 0 ~ 7 ) Define start page address
// interval:    ( 0 ~ 7 ) Set time interval between each scroll step in terms of frame frequency
// end_page:    ( 0 ~ 7 ) Define end page address
void oled_module::SSD_Set_Horizontal_Scroll(u8 LR, u8 start_page, u8 interval, u8 end_page)
{
    ssd1306_write_cmd(SSD_SET_HOR_R_SCROLL + LR); // Right Horizontal Scroll
    ssd1306_write_cmd(0x00); // Dummy byte (Set as 00h)
    ssd1306_write_cmd(start_page); // Define start page address
    ssd1306_write_cmd(interval); // Set time interval between each scroll step in terms of frame frequency
    ssd1306_write_cmd(end_page); // Define end page address
    ssd1306_write_cmd(0x00); // Dummy byte (Set as 00h)
    ssd1306_write_cmd(0xFF); // Dummy byte (Set as FFh)
}


/*----------------------------------------------------------------------------*/
// SSD1306
// LR:      0 = Vertical and Right Horizontal Scroll
//          1 = Vertical and Left  Horizontal Scroll
// start_page:  ( 0 ~ 7 ) Define start page address
// interval:    ( 0 ~ 7 ) Set time interval between each scroll step in terms of frame frequency
// end_page:    ( 0 ~ 7 ) Define end page address
// offset:      ( 1 ~ 38 ) Vertical scrolling offset
void oled_module::SSD_Set_Vertical_Scroll(u8 LR, u8 start_page, u8 interval, u8 end_page,
                             u8 offset)
{
    ssd1306_write_cmd(SSD_SET_VER_R_SCROLL + LR); // Vertical and Right Horizontal Scroll
    ssd1306_write_cmd(0x00); // Dummy byte (Set as 00h)
    ssd1306_write_cmd(start_page); // Define start page address
    ssd1306_write_cmd(interval); // Set time interval between each scroll step in terms of frame frequency
    ssd1306_write_cmd(end_page); // Define end page address
    ssd1306_write_cmd(offset); // Vertical scrolling offset
}

/*----------------------------------------------------------------------------*/
// SSD1306
// offset:  ( 0 ~ 39 ) Set No. of pages in top fixed area (RESET = 0 )
// pages:    ( 0 ~ 39 ) Set No. of pages in scroll area    (RESET = 39)
void oled_module::SSD_Set_Vertical_Scroll_Area(u8 offset, u8 pages)
{
    ssd1306_write_cmd(SSD_SET_VER_SCROLL_AREA); // Vertical scrolling offset
    ssd1306_write_cmd(offset); // Set No. of pages in top fixed area
    ssd1306_write_cmd(pages); // Set No. of pages in scroll area
}

/*----------------------------------------------------------------------------*/
// SSD1306
void oled_module::SSD_Set_Activate_Scroll(void)
{
    ssd1306_write_cmd(SSD_SET_ACTIVATE_SCROLL); // Activate Scrolling
}

/*----------------------------------------------------------------------------*/
// SSD1306
// pag:     ( 0 ~ 7   )
// col:     ( 0 ~ 127 )
void oled_module::SSD_Set_RAM_Address(u8 pag, u8 col)
{
//    Log.d(TAG,"set ram page %d col %d ",pag,col);
    SSD_Set_Start_Page(pag);
    SSD_Set_Start_Column(col);
}

void oled_module::fill(const u8 x, const u8 y, const u8 w, const u8 h,const u8 dat)
{

}

// SSD1306 fill display reg : SSD_RAM[5][128]
// dat:     fill data
// note:    fill page4

void oled_module::ssd1306_fill(const u8 dat)
{
    u8 pag, col;

#ifdef HORIZONAL_ADDRESS_MODE
    SSD_Set_RAM_Address(0,0);
#endif

    memcpy(ucBufLast,ucBuf,MAX_BUF);
    for (pag = 0; pag < PAGE_MAX; pag++)
    {
		#ifndef HORIZONAL_ADDRESS_MODE
        SSD_Set_RAM_Address(pag,0);
		#endif

        for (col = 0; col < COL_MAX; col++)
        {
            ssd1306_write_dat(dat);
        }
        memset(&ucBuf[pag * COL_MAX],dat,COL_MAX);
    }
}

void oled_module::ssd1306_set_off()
{
    ssd1306_write_cmd(0xAE);	/*display off*/
}

void oled_module::ssd1306_set_on()
{
    ssd1306_write_cmd(0xAF);	/*display off*/
}

void oled_module::ssd1306_init()
{
    ssd1306_reset();
    ssd1306_set_off();
    //init from datasheet order

    SSD_Set_Display_Clock(0x80);
    SSD_Set_Multiplex_Ratio(0x3F);
    SSD_Set_Display_Offset(0);
    SSD_Set_Start_Line(0);
    SSD_Set_Charge_Pump(CHARGE_PUMP_VAL);
    SSD_Set_Segment_Remap(1);
    SSD_Set_Common_Remap(1);
    SSD_Set_Segment_Config(0,1);
    SSD_Set_Contrast(0xCF);
    SSD_Set_Precharge_Period(0x1F);
    SSD_Set_VCOM_Level(0x30);
    SSD_Set_Entire_Display(0);
    SSD_Set_Inverse_Display(0);
    SSD_Set_Address_Mode(2);
    SSD_Set_Start_Page(0);
    SSD_Set_Start_Column(0);
    SSD_Set_Deactivate_Scroll();

    ssd1306_fill(0x00);
    ssd1306_set_on();

    //delay 100 according to datashee
    msg_util::sleep_ms(100);
}



/*
 * col : 0 - 127
 * page: 0 - 7
 * col_w : 0 - (COL_MAX - col)
 * page_h: 0 - (PAGE_MAX - page)
 */

void oled_module::disp_dat(u8 col, u8 page, u8 col_w,u8 page_num)
{
//    Log.d(TAG,"col %d page %d col_w %d page_num %d\n",
//           col,page, col_w, page_num);
#ifdef HORIZONAL_ADDRESS_MODE
    //    SSD_Set_RAM_Address(page, col);
    SSD_Set_Page_Address(page,page + page_num);
    SSD_Set_Column_Address(col,col + col_w);
#endif

    for (u8 i = 0; i < page_num; i++)
    {
        u16 start = (page + i) * COL_MAX + col;

        #ifndef HORIZONAL_ADDRESS_MODE
        SSD_Set_RAM_Address(page + i,col);
		#endif

        for (u8 j = 0; j < col_w; j++ )
        {
            ssd1306_write_dat(ucBuf[start + j]);
        }
    }
}

void oled_module::disp_buf_last()
{
#ifdef HORIZONAL_ADDRESS_MODE
    SSD_Set_Page_Address(0,ROW_MAX);
    SSD_Set_Column_Address(0, COL_MAX);
#endif

    for (u8 i = 0; i < ROW_MAX; i++)
    {
        u16 start = i * COL_MAX;

        #ifndef HORIZONAL_ADDRESS_MODE
        SSD_Set_RAM_Address(i,start);
		#endif

        for (u8 j = 0; j < COL_MAX; j++ )
        {
            ssd1306_write_dat(ucBuf[start + j]);
        }
    }
}

//page:0-7 col:0-127
void oled_module::set_buf(u8 col, u8 page, u8 dat)
{
    ucBuf[page * COL_MAX + col] = dat;
}


//page:0-7 col:0-127
void oled_module::set_buf(u8 col,u8 page, const u8 *dat, u8 len)
{
//    Log.d(TAG,"set buf page %d col %d len %d\n",page, col,len);
//    Log.d(TAG,"set buf[%d] len %d\n", page * COL_MAX + col, len);
//    Log.d(TAG,"\n");
    memcpy(&ucBuf[page * COL_MAX + col],dat, len);
}

void oled_module::set_buf(const u8 dat,u8 col, u8 page_start, u8 col_w,u8 page_num)
{
    for(u8 page = 0; page < page_num; page++)
    {
        memset(&ucBuf[(page_start + page) * COL_MAX + col],dat,col_w);
    }
}

/* y: 0-63, h%8 must be 0
 */
void oled_module::get_page_info_convert(u8 y,u8 h,sp<PAGE_INFO> &mPageInfo)
{
    mPageInfo->delta_y = y%PAGE_H;
    u8 delta1 = h%PAGE_H;
    CHECK_EQ(delta1,0);
    mPageInfo->page_start = y/PAGE_H;
    mPageInfo->dat_page_num = h/PAGE_H;

    if (mPageInfo->delta_y != 0)
    {
        mPageInfo->page_num = mPageInfo->dat_page_num + 1;
        if (mPageInfo->page_num  >= 4)
        {
            Log.d(TAG,"warning: page occupy more than 4\n");
        }
    }
    else
    {
        mPageInfo->page_num = mPageInfo->dat_page_num;
    }
}


//high: 0 - normal ,1 -- highlight
void oled_module::get_char_info(u8 c,sp<CHAR_INFO> &mCharInfo, bool high)
{
    u32 i = 0;
    u32 total = sizeof(others_array)/ sizeof(others_array[0]);

    if (c >= '0' && c <= '9')
    {
        mCharInfo->char_w = 6;
        mCharInfo->pucChar = (const u8 *)&digit_array[high][c - '0'];
    }
    else if ( c >= 'A' && c <= 'Z')
    {
        mCharInfo->char_w = 8;
        mCharInfo->pucChar = (const u8 *)&alpha_bigger[high][c - 'A'];
    }
    else if ( c >= 'a' && c <= 'z')
    {
        mCharInfo->char_w = 8;
        mCharInfo->pucChar = (const u8 *)&alpha_smaller[high][c - 'a'];
    }
    else
    {
        for (i = 0; i < total; i++)
        {
            if (others_array[i].c == c)
            {
                mCharInfo->char_w = 6;
                mCharInfo->pucChar = others_array[i].pucChar[high];
                break;
            }
        }

        if (i == total)
        {
            Log.e(TAG, "not found char 0x%x", c);
            mCharInfo->char_w = 6;
            mCharInfo->pucChar = others_array[0].pucChar[high];
        }
    }
}


// used while start y%PAGE_H != 0
void oled_module::set_buf_by_page_info(u8 col_start, DAT_TYPE ucMid,sp<PAGE_INFO> mPageInfo)
{
    u8 ucFirst = (u8) (ucBuf[col_start + mPageInfo->page_start * COL_MAX ] & (0xff >> (PAGE_H - mPageInfo->delta_y)));
    u8 ucLast = (u8) (ucBuf[col_start + (mPageInfo->page_start + mPageInfo->dat_page_num) * COL_MAX] & (u8(0xff << mPageInfo->delta_y)));
    DAT_TYPE ucDat = (DAT_TYPE)((ucLast << (mPageInfo->dat_page_num * 8)) | (ucMid << mPageInfo->delta_y) | ucFirst);
//    Log.d(TAG,"0x%x | " DAT_FORMAT " | 0x%x ucDat " DAT_FORMAT "\n",
//            ucLast, ucMid, ucFirst,ucDat);
    for (u8 k = 0; k < mPageInfo->page_num; k++)
    {
//                    Log.d(TAG,"[%d] 0x%x ",
//                           col_start + (mPageInfo->page_start + k) * COL_MAX,
//                           (u8)((ucDat >> (8 *k)) & 0xff));
//                        ucBuf[col_start + (mPageInfo->page_start + k) * COL_MAX] = (u8) ((ucDat >> (8 * k)) & 0xff);
//                            (u8)((ucDat >> ( 8 * k) ) & 0xff);
//                             (u8)((ucDat >> (8 *k)) & 0xff));
//                    Log.d(TAG,"%d", col_start + (mPageInfo->page_start + k) * COL_MAX );
//                    Log.d(TAG,"\n");
        set_buf(col_start ,mPageInfo->page_start + k,(u8) ((ucDat >> (8 * k)) & 0xff));
    }
}

void oled_module::ssd1306_disp_16_str_fill(const u8 *str, const u8 x, const u8 y, bool bHigh)
{
    ssd1306_disp_16_str(str,x,y,bHigh,COL_MAX - x);
}

/*x: 0-127 y:0 - 48
 *
 * attention:
 * 1 str that all the bytes height are 16
 * 2 if str width less than width(if set),fill 0xff if bHigh or 0x00
 */
void oled_module::ssd1306_disp_16_str(const u8 *str,const u8 x, const u8 y, bool bHigh, u8 width )
{
    u8 len = (u8)strlen((int8 *)str);
    u8 col_start = x;
    u8 col_limit = COL_MAX;


    if(width > 0)
    {
        col_limit = ((x + width) < COL_MAX )? (x + width):COL_MAX;
    }
    if (y + char_h > ROW_MAX )
    {
        Log.d(TAG,"page %d exceed %d\n",y + char_h,ROW_MAX);
#ifdef ENABLE_ABORT
        abort();
#else
        return;
#endif
    }

    sp<CHAR_INFO> mCharInfo = sp<CHAR_INFO>(new CHAR_INFO());
    get_page_info_convert(y,char_h,mPageInfo);

//    Log.d(TAG,"str is %s len %d x %d y %d col_start %d "
//                  "col_limit %d width %d bHigh %d\n",
//          str, len,x, y, col_start,col_limit, width,bHigh);
//    Log.d(TAG,"x %d w %d col_limit %d str %s",x,width,col_limit,str);
    if ( mPageInfo->delta_y != 0)
    {
        DAT_TYPE ucMid = 0;
        for(u8 i = 0; i < len ; i++)
        {
            get_char_info(str[i],mCharInfo,bHigh);
            if((col_start + mCharInfo->char_w) <= col_limit )
            {
                for (u8 j = 0; j < mCharInfo->char_w; j++)
                {
                    ucMid = 0;
                    for (u8 page = 0; page < mPageInfo->dat_page_num; page++)
                    {
                        ucMid |= mCharInfo->pucChar[page * mCharInfo->char_w + j] << (8 * page);
                    }
                    set_buf_by_page_info(col_start,ucMid,mPageInfo);
                    col_start++;
                }
            }
            else
            {
                Log.d(TAG,"***exceed %d\n",
                       col_start + mCharInfo->char_w);
                goto EXIT;
            }
        }
        // fill dat if need,use width nor col_limit for width might be zero
        if(width > 0 && (col_start - x) < col_limit)
        {
            u8 f_dat = fill_dat[bHigh];
//            Log.d(TAG,"c 0x%x\n",f_dat);
            while(col_start < col_limit)
            {
                ucMid = 0;
                for (u8 page = 0; page < mPageInfo->dat_page_num; page++)
                {
                    ucMid |= f_dat << (8 * page);
                }
                set_buf_by_page_info(col_start,ucMid,mPageInfo);
                col_start++;
            }
        }
    }
    else
    {
        for(u8 i = 0; i < len; i++)
        {
            if((col_start + mCharInfo->char_w) <= col_limit)
            {
                get_char_info(str[i],mCharInfo,bHigh);
                for(u8 page = 0; page < mPageInfo->page_num; page++)
                {
                    set_buf(col_start,(mPageInfo->page_start + page),
                            (const u8 *)&mCharInfo->pucChar[page * mCharInfo->char_w],mCharInfo->char_w);
                }
                col_start += mCharInfo->char_w;
            }
            else
            {
                Log.d(TAG,"!!!exceed %d\n",col_start + mCharInfo->char_w);
                goto EXIT;
            }
        }
//        Log.d(TAG,"col_start is %d x %d col_limit %d", col_start,x,col_limit);
        // fill dat if need use width nor col_limit for width might be zero
        if(width > 0 && (col_start - x) < col_limit)
        {
//            Log.d(TAG," fill dat 0x%x col_start %d col_limit %d"
//                          " mPageInfo->page_start %d mPageInfo->page_num %d",
//                  fill_dat[bHigh], col_start,col_limit, mPageInfo->page_start,
//                  mPageInfo->page_num);
            // use col_limit not width for width + x may exceed COL_MAX
            set_buf(fill_dat[bHigh],col_start,mPageInfo->page_start,
                    col_limit - col_start,mPageInfo->page_num);
            col_start = col_limit;
        }
    }
EXIT:
//    Log.d(TAG,"disp x %d w %d",x,col_start -x );
    disp_dat(x,mPageInfo->page_start,
             col_start - x ,mPageInfo->page_num);
}

void oled_module::clear_icon(const u32 icon_type)
{
    const struct _icon_info_ *pstTmp = (const struct _icon_info_ *)&oled_icons[icon_type];
//    Log.d(TAG,"clear_icon icon_type %d",icon_type);
    clear_area(pstTmp->x,pstTmp->y,pstTmp->w,pstTmp->h);
}

void oled_module::disp_icon(const u32 icon_type)
{
    if (icon_type > ICON_MAX)
    {
        Log.e(TAG,"disp_icon exceed icon_type %d",icon_type);
#ifdef ENABLE_ABORT
        abort();
#else
        return;
#endif
    }
    else if (icon_type == 0)
    {
        Log.w(TAG, "disp icon 0 happen");
    }
    disp_icon((const struct _icon_info_ *)&oled_icons[icon_type]);
}

void oled_module::disp_icon(const struct _icon_info_ *pstTmp)
{
//    Log.d(TAG,"disp len %d",pstTmp->size);
    ssd1306_disp_icon(pstTmp->dat, pstTmp->x, pstTmp->y, pstTmp->w, pstTmp->h);
}

void oled_module::disp_ip(const u8 *ip)
{
    ssd1306_disp_16_str(ip,12,0,0,90);
}

/*x: 0-127 y:0 - 63 w:128 - x h:64 - y
 *
 */

void oled_module::ssd1306_disp_icon(const u8 *dat,const u8 x, const u8 y,const u8 w,const u8 h)
{
//    Log.d(TAG,"ssd1306_disp_icon (%d %d %d %d)", x,y,w,h);
    u8 width = w;
    u8 height = h;

    CHECK_NE(width,0);
    CHECK_NE(height,0);

    if( (x + width) > COL_MAX)
    {
//        Log.e(TAG,"disp icon x+w %d exceed\n",x + w);
        width--;
    }
	
    while ((y + height) > ROW_MAX)
    {
//        Log.e(TAG,"disp icon y+h %d exceed\n",y + h);
        height -= PAGE_H;
    }
	
    get_page_info_convert(y,height,mPageInfo);

    if (mPageInfo->delta_y != 0)
    {
        DAT_TYPE ucMid = 0;
        u8 col_start = x;
        for (u8 i = 0; i < w ; i++)
        {
            ucMid = 0;
            for (u8 page = 0; page < mPageInfo->dat_page_num; page++)
            {
                ucMid |= dat[page * w + i] << (8 * page);
            }
            set_buf_by_page_info(col_start,ucMid,mPageInfo);
            col_start++;
        }
    }
    else
    {
        for (u8 i = 0; i < mPageInfo->page_num; i++)
        {
            set_buf(x,(mPageInfo->page_start + i),
                    (const u8 *)&dat[i * w],w);
        }
    }
    disp_dat(x,mPageInfo->page_start, width ,mPageInfo->page_num);
}

void oled_module::clear_area_w(const u8 x,const u8 y,const u8 w)
{
    clear_area(x, y, w, (ROW_MAX - y ));
}


void oled_module::display_onoff(u8 sw)
{
    if (sw) {
		SSD_Set_Display_ON_OFF(1);
    } else {
		SSD_Set_Display_ON_OFF(0);
	}
}


void oled_module::clear_area_h(const u8 x,const u8 y,const u8 h)
{
    clear_area(x, y,(COL_MAX - x),h);
}

void oled_module::clear_area(const u8 x,const u8 y)
{
    CHECK_EQ(y%PAGE_H,0);
    clear_area(x, y,(COL_MAX - x),(ROW_MAX - y ));
}

//both y%PAGE_H and h%PAGE_H should be 0
void oled_module::clear_area(const u8 x,const u8 y,const u8 w,const u8 h)
{
    CHECK_EQ(y%PAGE_H,0);
    CHECK_EQ(h%PAGE_H,0);
    clear_area_page(x, y/PAGE_H,w,h/PAGE_H);
}

void oled_module::clear_area_page(const u8 col,const u8 page,const u8 col_w,const u8 page_num)
{
    u8 dat = 0x00;
//    Log.d(TAG,"col %d page %d col_w %d page_num %d",col,page,col_w,page_num);

    for (u8 i = 0; i < page_num; i++)
    {
        SSD_Set_RAM_Address(page + i,col);
        for (u8 j = 0; j < col_w; j++)
        {
            ssd1306_write_dat(dat);
        }
        memset(&ucBuf[col + (page + i) * COL_MAX],dat,col_w);
    }
}

void oled_module::start_horizon_scroll(const u8 col_start,const u8 page_start,const u8 col_w,const u8 page_num)
{
    u8 first_byte = 0;
    const u8 copy_w = col_w -1;
    u8 src = 0;
    u8 dest = 0;
    u8 last_pos = 0;
    while(1)
    {
        for(u8 page = 0; page < page_num; page++)
        {
            dest = col_start + (page_start + page) * COL_MAX;
            src = dest + 1;
            last_pos = dest + col_w - 1;
            first_byte = ucBuf[dest];
//            Log.d(TAG,"dest %d src %d copy_w %d last_pos %d\n",dest,src,copy_w, last_pos);
            memmove(&ucBuf[dest],&ucBuf[src],copy_w);
            ucBuf[last_pos] = first_byte;
        }
        disp_dat(col_start,page_start,col_w,page_num);
    }
}


void oled_module::ssd1306_write_cmd(const u8 cmd)
{
    pstI2C_OLED->i2c_write_byte(SSD_CMD_ADDRESS, cmd);
}

void oled_module::ssd1306_write_dat(u8 dat)
{
    pstI2C_OLED->i2c_write_byte(SSD_DAT_ADDRESS, dat);
}

