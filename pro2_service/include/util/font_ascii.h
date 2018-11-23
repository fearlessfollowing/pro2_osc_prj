#ifndef FONT_ASCII_H
#define FONT_ASCII_H

#include <sys/ins_types.h>

typedef struct _char_info_ {
    u8 char_w;
    const u8 *pucChar;
} CHAR_INFO;

typedef struct _char_other_ {
    u8 c;
    const u8 *pucChar[2];
} CHAR_OTHER;


typedef struct _coordinate_ {
    const u8 x;
    const u8 y;
//    const u8 w;
//    const u8 h;
}COORDINATE;

enum {
    INDEX_IP,
};

static const COORDINATE mCoordinate[] = {
    {20, 2}
};

#if 0
static u8 InsDev_Oled_Char_colon_bin[] = {
    /*    6x16    */
    0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,
};

static u8 InsDev_Oled_Char_minus_bin[] = {
    /*    4x16    */
    0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,
};

static u8 InsDev_Oled_Char_dot_bin[] = {
    /*    4x16    */
    0x00,0x00,0x00,0x00,0x0C,0x00,
};

static u8 InsDev_Oled_Char_question_bin[] = {
    /*    6x16    */
    0x00,0x30,0x08,0x88,0x88,0x70,0x00,0x00,0x00,0x0D,0x00,0x00,
};
#endif

static u8 InsDev_Oled_Char_A_bin[] = {
    /*    8x16    */
    0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,
};

static u8 InsDev_Oled_Char_B_bin[] = {
    /*    7x16    */
    0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x0F,0x08,0x08,0x08,0x08,0x07,
};

static u8 InsDev_Oled_Char_C_bin[] = {
    /*    7x16    */
    0x00,0xE0,0x10,0x10,0x10,0x10,0x20,0x00,0x07,0x08,0x08,0x08,0x08,0x04,
};

static u8 InsDev_Oled_Char_D_bin[] = {
    /*    8x16    */
    0x00,0xF0,0x10,0x10,0x10,0x10,0x20,0xC0,0x00,0x0F,0x08,0x08,0x08,0x08,0x04,0x03,
};

static u8 InsDev_Oled_Char_E_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x90,0x90,0x90,0x90,0x00,0x0F,0x08,0x08,0x08,0x08,
};

static u8 InsDev_Oled_Char_F_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0x0F,0x00,0x00,0x00,0x00,
};

static u8 InsDev_Oled_Char_G_bin[] = {
    /*    7x16    */
    0x00,0xE0,0x10,0x10,0x10,0x10,0x20,0x00,0x07,0x08,0x08,0x08,0x09,0x0F,
};

static u8 InsDev_Oled_Char_H_bin[] = {
    /*    7x16    */
    0x00,0xF0,0x80,0x80,0x80,0x80,0xF0,0x00,0x0F,0x00,0x00,0x00,0x00,0x0F,
};

static u8 InsDev_Oled_Char_I_bin[] = {
    /*    4x16    */
    0x00,0x10,0xF0,0x10,0x00,0x08,0x0F,0x08,
};

static u8 InsDev_Oled_Char_J_bin[] = {
    /*    4x16    */
    0x00,0x00,0x00,0xF0,0x00,0x20,0x20,0x1F,
};

static u8 InsDev_Oled_Char_K_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0x0F,0x01,0x02,0x04,0x08,
};

static u8 InsDev_Oled_Char_L_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x0F,0x08,0x08,0x08,0x08,
};

static u8 InsDev_Oled_Char_M_bin[] = {
    /*    8x16    */
    0x00,0xF0,0x30,0xC0,0x00,0xC0,0x30,0xF0,0x00,0x0F,0x00,0x00,0x03,0x00,0x00,0x0F,
};

static u8 InsDev_Oled_Char_N_bin[] = {
    /*    7x16    */
    0x00,0xF0,0x20,0x40,0x80,0x00,0xF0,0x00,0x0F,0x00,0x00,0x00,0x01,0x0F,
};

static u8 InsDev_Oled_Char_O_bin[] = {
    /*    8x16    */
    0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0x03,0x04,0x08,0x08,0x08,0x04,0x03,
};

static u8 InsDev_Oled_Char_P_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x0F,0x01,0x01,0x01,0x00,
};

static u8 InsDev_Oled_Char_Q_bin[] = {
    /*    8x16    */
    0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0x03,0x04,0x08,0x08,0x08,0x14,0x03,
};

static u8 InsDev_Oled_Char_R_bin[] = {
    /*    8x16    */
    0x00,0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x0F,0x01,0x01,0x03,0x04,0x08,
};

static u8 InsDev_Oled_Char_S_bin[] = {
    /*    8x16    */
    0x00,0x60,0x90,0x90,0x90,0x10,0x00,0x08,0x08,0x08,0x08,0x07,
};

static u8 InsDev_Oled_Char_T_bin[] = {
    /*    8x16    */
    0x00,0x10,0x10,0x10,0xF0,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,
};

static u8 InsDev_Oled_Char_U_bin[] = {
    /*    8x16    */
    0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x07,0x08,0x08,0x08,0x08,0x07,
};

static u8 InsDev_Oled_Char_V_bin[] = {
    /*    8x16    */
    0x00,0x30,0xC0,0x00,0x00,0x00,0xC0,0x30,0x00,0x00,0x00,0x03,0x0C,0x03,0x00,0x00,
};

static u8 InsDev_Oled_Char_W_bin[] = {
    /*   10x16    */
    0x00,0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x03,0x0C,0x03,0x00,
    0x03,0x0C,0x03,0x00,
};

static u8 InsDev_Oled_Char_X_bin[] = {
    /*    7x16    */
    0x00,0x30,0x40,0x80,0x80,0x40,0x30,0x00,0x0C,0x03,0x00,0x00,0x03,0x0C,
};

static u8 InsDev_Oled_Char_Y_bin[] = {
    /*    8x16    */
    0x00,0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,
};

static u8 InsDev_Oled_Char_Z_bin[] = {
    /*    8x16    */
    0x00,0x10,0x10,0x10,0x90,0x50,0x30,0x10,0x00,0x0C,0x0A,0x09,0x08,0x08,0x08,0x08,
};

static u8 InsDev_Oled_Char_a_bin[] = {
    /*    6x16    */
    0x00,0x80,0x40,0x40,0x40,0x80,0x00,0x06,0x09,0x09,0x09,0x0F,
};

static u8 InsDev_Oled_Char_b_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x40,0x40,0x40,0x80,0x00,0x0F,0x08,0x08,0x08,0x07,
};

static u8 InsDev_Oled_Char_c_bin[] = {
    /*    5x16    */
    0x00,0x80,0x40,0x40,0x40,0x00,0x07,0x08,0x08,0x08,
};

static u8 InsDev_Oled_Char_d_bin[] = {
    /*    6x16    */
    0x00,0x80,0x40,0x40,0x40,0xF0,0x00,0x07,0x08,0x08,0x08,0x0F,
};

static u8 InsDev_Oled_Char_e_bin[] = {
    /*    6x16    */
    0x00,0x80,0x40,0x40,0x40,0x80,0x00,0x07,0x09,0x09,0x09,0x09,
};

static u8 InsDev_Oled_Char_f_bin[] = {
    /*    4x16    */
    0x00,0xE0,0x50,0x50,0x00,0x0F,0x00,0x00,
};

static u8 InsDev_Oled_Char_g_bin[] = {
    /*    6x16    */
    0x00,0x80,0x40,0x40,0x40,0xC0,0x00,0x07,0x28,0x28,0x28,0x1F,
};

static u8 InsDev_Oled_Char_h_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x40,0x40,0x40,0x80,0x00,0x0F,0x00,0x00,0x00,0x0F,
};

static u8 InsDev_Oled_Char_i_bin[] = {
    /*    2x16    */
    0x00,0xD0,0x00,0x0F,
};

static u8 InsDev_Oled_Char_j_bin[] = {
    /*    3x16    */
    0x00,0x00,0xD0,0x00,0x20,0x3F,
};

static u8 InsDev_Oled_Char_k_bin[] = {
    /*    6x16    */
    0x00,0xF0,0x00,0x80,0x40,0x00,0x00,0x0F,0x01,0x02,0x04,0x08,
};

static u8 InsDev_Oled_Char_l_bin[] = {
    /*    2x16    */
    0x00,0xF0,0x00,0x0F,
};

static u8 InsDev_Oled_Char_m_bin[] = {
    /*    8x16    */
    0x00,0xC0,0x40,0x40,0x80,0x40,0x40,0x80,0x00,0x0F,0x00,0x00,0x0F,0x00,0x00,0x0F,
};

static u8 InsDev_Oled_Char_n_bin[] = {
    /*    6x16    */
    0x00,0xC0,0x40,0x40,0x40,0x80,0x00,0x0F,0x00,0x00,0x00,0x0F,
};

static u8 InsDev_Oled_Char_o_bin[] = {
    /*    6x16    */
    0x00,0x80,0x40,0x40,0x40,0x80,0x00,0x07,0x08,0x08,0x08,0x07,
};

static u8 InsDev_Oled_Char_p_bin[] = {
    /*    6x16    */
    0x00,0xC0,0x40,0x40,0x40,0x80,0x00,0x3F,0x08,0x08,0x08,0x07,
};

static u8 InsDev_Oled_Char_q_bin[] = {
    /*    6x16    */
    0x00,0x80,0x40,0x40,0x40,0xC0,0x00,0x07,0x08,0x08,0x08,0x3F,
};

static u8 InsDev_Oled_Char_r_bin[] = {
    /*    4x16    */
    0x00,0xC0,0x40,0x40,0x00,0x0F,0x00,0x00,
};

static u8 InsDev_Oled_Char_s_bin[] = {
    /*    6x16    */
    0x00,0x80,0x40,0x40,0x40,0x80,0x00,0x04,0x09,0x09,0x0A,0x04,
};

static u8 InsDev_Oled_Char_t_bin[] = {
    /*    4x16    */
    0x00,0xF0,0x40,0x40,0x00,0x07,0x08,0x08,
};

static u8 InsDev_Oled_Char_u_bin[] = {
    /*    6x16    */
    0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x07,0x08,0x08,0x08,0x0F,
};

static u8 InsDev_Oled_Char_v_bin[] = {
    /*    6x16    */
    0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00,0x03,0x0C,0x03,0x00,
};

static u8 InsDev_Oled_Char_w_bin[] = {
    /*    8x16    */
    0x00,0xC0,0x00,0x00,0xC0,0x00,0x00,0xC0,0x00,0x03,0x0C,0x03,0x00,0x03,0x0C,0x03,
};

static u8 InsDev_Oled_Char_x_bin[] = {
    /*    6x16    */
    0x00,0x40,0x80,0x00,0x80,0x40,0x00,0x0C,0x02,0x01,0x02,0x0C,
};

static u8 InsDev_Oled_Char_y_bin[] = {
    /*    6x16    */
    0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00,0x33,0x0C,0x03,0x00,
};

static u8 InsDev_Oled_Char_z_bin[] = {
    /*    6x16    */
    0x00,0x40,0x40,0x40,0x40,0xC0,0x00,0x08,0x0C,0x0A,0x09,0x08,
};



static u8 InsDev_Oled_Char_empty_bin[] = {
    /*    6x16    */
    0x00,0x00,0x00,0x00,0x00,0x00,
};


#if 0
InsDev_Oled_Char_t InsDev_Oled_Char[] = {
   { 3, InsDev_Oled_Char_empty_bin}, //sp 0
   { 3, InsDev_Oled_Char_empty_bin}, // ! 1
   { 3, InsDev_Oled_Char_empty_bin}, // " 2
   { 3, InsDev_Oled_Char_empty_bin}, // # 3
   { 3, InsDev_Oled_Char_empty_bin}, // $ 4
   { 3, InsDev_Oled_Char_empty_bin}, // % 5
   { 3, InsDev_Oled_Char_empty_bin}, // & 6
   { 3, InsDev_Oled_Char_empty_bin}, // ' 7
   { 3, InsDev_Oled_Char_empty_bin}, // ( 8
   { 3, InsDev_Oled_Char_empty_bin}, // ) 9
   { 3, InsDev_Oled_Char_empty_bin}, // * 10
   { 3, InsDev_Oled_Char_empty_bin}, // + 11
   { 3, InsDev_Oled_Char_empty_bin}, // , 12
   { 4, InsDev_Oled_Char_minus_bin}, // - 13
   { 3, InsDev_Oled_Char_dot_bin}, // . 14
   { 3, InsDev_Oled_Char_empty_bin}, // / 15
   { 6, InsDev_Oled_Char_0_bin}, // 0 16
   { 6, InsDev_Oled_Char_1_bin}, // 1 17
   { 6, InsDev_Oled_Char_2_bin}, // 2 18
   { 6, InsDev_Oled_Char_3_bin}, // 3 19
   { 6, InsDev_Oled_Char_4_bin}, // 4 20
   { 6, InsDev_Oled_Char_5_bin}, // 5 21
   { 6, InsDev_Oled_Char_6_bin}, // 6 22
   { 6, InsDev_Oled_Char_7_bin}, // 7 23
   { 6, InsDev_Oled_Char_8_bin}, // 8 24
   { 6, InsDev_Oled_Char_9_bin}, // 9 25
   { 6, InsDev_Oled_Char_colon_bin}, // : 26
   { 3, InsDev_Oled_Char_empty_bin}, // ; 27
   { 3, InsDev_Oled_Char_empty_bin}, // < 28
   { 3, InsDev_Oled_Char_empty_bin}, // = 29
   { 3, InsDev_Oled_Char_empty_bin}, // > 30
   { 6, InsDev_Oled_Char_question_bin}, // ? 31
   { 3, InsDev_Oled_Char_empty_bin}, // @ 32
   { 8, InsDev_Oled_Char_A_bin}, // A 33
   { 7, InsDev_Oled_Char_B_bin}, // B 34
   { 7, InsDev_Oled_Char_C_bin}, // C 35
   { 8, InsDev_Oled_Char_D_bin}, // D 36
   { 6, InsDev_Oled_Char_E_bin}, // E 37
   { 6, InsDev_Oled_Char_F_bin}, // F 38
   { 7, InsDev_Oled_Char_G_bin}, // G 39
   { 7, InsDev_Oled_Char_H_bin}, // H 40
   { 4, InsDev_Oled_Char_I_bin}, // I 41
   { 4, InsDev_Oled_Char_J_bin}, // J 42
   { 6, InsDev_Oled_Char_K_bin}, // K 43
   { 6, InsDev_Oled_Char_L_bin}, // L 44
   { 8, InsDev_Oled_Char_M_bin}, // M 45
   { 7, InsDev_Oled_Char_N_bin}, // N 46
   { 8, InsDev_Oled_Char_O_bin}, // O 47
   { 6, InsDev_Oled_Char_P_bin}, // P 48
   { 8, InsDev_Oled_Char_Q_bin}, // Q 49
   { 8, InsDev_Oled_Char_R_bin}, // R 50
   { 6, InsDev_Oled_Char_S_bin}, // S 51
   { 8, InsDev_Oled_Char_T_bin}, // T 52
   { 8, InsDev_Oled_Char_U_bin}, // U 53
   { 8, InsDev_Oled_Char_V_bin}, // V 54
   { 10, InsDev_Oled_Char_W_bin}, // W 55
   { 7, InsDev_Oled_Char_X_bin}, // X 56
   { 8, InsDev_Oled_Char_Y_bin}, // Y 57
   { 8, InsDev_Oled_Char_Z_bin}, // Z 58
   { 3, InsDev_Oled_Char_empty_bin}, // [ 59
   { 3, InsDev_Oled_Char_empty_bin}, // \ 60
   { 3, InsDev_Oled_Char_empty_bin}, // ] 61
   { 3, InsDev_Oled_Char_empty_bin}, // ^ 62
   { 3, InsDev_Oled_Char_empty_bin}, // _ 63
   { 3, InsDev_Oled_Char_empty_bin}, // ' 64
   { 6, InsDev_Oled_Char_a_bin}, // a 65
   { 6, InsDev_Oled_Char_b_bin}, // b 66
   { 5, InsDev_Oled_Char_c_bin}, // c 67
   { 6, InsDev_Oled_Char_d_bin}, // d 68
   { 6, InsDev_Oled_Char_e_bin}, // e 69
   { 4, InsDev_Oled_Char_f_bin}, // f 70
   { 6, InsDev_Oled_Char_g_bin}, // g 71
   { 6, InsDev_Oled_Char_h_bin}, // h 72
   { 2, InsDev_Oled_Char_i_bin}, // i 73
   { 3, InsDev_Oled_Char_j_bin}, // j 74
   { 6, InsDev_Oled_Char_k_bin}, // k 75
   { 2, InsDev_Oled_Char_l_bin}, // l 76
   { 8, InsDev_Oled_Char_m_bin}, // m 77
   { 6, InsDev_Oled_Char_n_bin}, // n 78
   { 6, InsDev_Oled_Char_o_bin}, // o 79
   { 6, InsDev_Oled_Char_p_bin}, // p 80
   { 6, InsDev_Oled_Char_q_bin}, // q 81
   { 4, InsDev_Oled_Char_r_bin}, // r 82
   { 6, InsDev_Oled_Char_s_bin}, // s 83
   { 4, InsDev_Oled_Char_t_bin}, // t 84
   { 6, InsDev_Oled_Char_u_bin}, // u 85
   { 6, InsDev_Oled_Char_v_bin}, // v 86
   { 8, InsDev_Oled_Char_w_bin}, // w 87
   { 6, InsDev_Oled_Char_x_bin}, // x 88
   { 6, InsDev_Oled_Char_y_bin}, // y 89
   { 6, InsDev_Oled_Char_z_bin}, // z 90
   { 3, InsDev_Oled_Char_empty_bin} // horiz lines91
};

#else

static CHAR_INFO gArrySmallAlpha[] = {
   { 6, InsDev_Oled_Char_a_bin}, // a 65
   { 6, InsDev_Oled_Char_b_bin}, // b 66
   { 5, InsDev_Oled_Char_c_bin}, // c 67
   { 6, InsDev_Oled_Char_d_bin}, // d 68
   { 6, InsDev_Oled_Char_e_bin}, // e 69
   { 4, InsDev_Oled_Char_f_bin}, // f 70
   { 6, InsDev_Oled_Char_g_bin}, // g 71
   { 6, InsDev_Oled_Char_h_bin}, // h 72
   { 2, InsDev_Oled_Char_i_bin}, // i 73
   { 3, InsDev_Oled_Char_j_bin}, // j 74
   { 6, InsDev_Oled_Char_k_bin}, // k 75
   { 2, InsDev_Oled_Char_l_bin}, // l 76
   { 8, InsDev_Oled_Char_m_bin}, // m 77
   { 6, InsDev_Oled_Char_n_bin}, // n 78
   { 6, InsDev_Oled_Char_o_bin}, // o 79
   { 6, InsDev_Oled_Char_p_bin}, // p 80
   { 6, InsDev_Oled_Char_q_bin}, // q 81
   { 4, InsDev_Oled_Char_r_bin}, // r 82
   { 6, InsDev_Oled_Char_s_bin}, // s 83
   { 4, InsDev_Oled_Char_t_bin}, // t 84
   { 6, InsDev_Oled_Char_u_bin}, // u 85
   { 6, InsDev_Oled_Char_v_bin}, // v 86
   { 8, InsDev_Oled_Char_w_bin}, // w 87
   { 6, InsDev_Oled_Char_x_bin}, // x 88
   { 6, InsDev_Oled_Char_y_bin}, // y 89
   { 6, InsDev_Oled_Char_z_bin}, // z 90        
};


static CHAR_INFO gArryBigAlpha[] = {
   { 8, InsDev_Oled_Char_A_bin}, // A 33
   { 7, InsDev_Oled_Char_B_bin}, // B 34
   { 7, InsDev_Oled_Char_C_bin}, // C 35
   { 8, InsDev_Oled_Char_D_bin}, // D 36
   { 6, InsDev_Oled_Char_E_bin}, // E 37
   { 6, InsDev_Oled_Char_F_bin}, // F 38
   { 7, InsDev_Oled_Char_G_bin}, // G 39
   { 7, InsDev_Oled_Char_H_bin}, // H 40
   { 4, InsDev_Oled_Char_I_bin}, // I 41
   { 4, InsDev_Oled_Char_J_bin}, // J 42
   { 6, InsDev_Oled_Char_K_bin}, // K 43
   { 6, InsDev_Oled_Char_L_bin}, // L 44
   { 8, InsDev_Oled_Char_M_bin}, // M 45
   { 7, InsDev_Oled_Char_N_bin}, // N 46
   { 8, InsDev_Oled_Char_O_bin}, // O 47
   { 6, InsDev_Oled_Char_P_bin}, // P 48
   { 8, InsDev_Oled_Char_Q_bin}, // Q 49
   { 8, InsDev_Oled_Char_R_bin}, // R 50
   { 6, InsDev_Oled_Char_S_bin}, // S 51
   { 8, InsDev_Oled_Char_T_bin}, // T 52
   { 8, InsDev_Oled_Char_U_bin}, // U 53
   { 8, InsDev_Oled_Char_V_bin}, // V 54
   { 10, InsDev_Oled_Char_W_bin}, // W 55
   { 7, InsDev_Oled_Char_X_bin}, // X 56
   { 8, InsDev_Oled_Char_Y_bin}, // Y 57
   { 8, InsDev_Oled_Char_Z_bin}, // Z 58      
};

#endif


static const u8 digit_6_16[][12] = {
    {0x00,0xE0,0x10,0x10,0x10,0xE0,0x00,0x07,0x08,0x08,0x08,0x07},//0 0
    {0x00,0x10,0x10,0xF0,0x00,0x00,0x00,0x08,0x08,0x0F,0x08,0x08},//1 1
    {0x00,0x20,0x10,0x10,0x10,0xE0,0x00,0x08,0x0C,0x0A,0x09,0x08},//2 2
    {0x00,0x10,0x90,0x90,0x90,0x60,0x00,0x08,0x08,0x08,0x08,0x07},//3 3
    {0x00,0x80,0x60,0x10,0xF0,0x00,0x00,0x03,0x02,0x02,0x0F,0x02},//4 4
    {0x00,0xF0,0x90,0x90,0x90,0x00,0x00,0x08,0x08,0x08,0x08,0x07},//5 5
    {0x00,0xE0,0x90,0x90,0x90,0x00,0x00,0x07,0x08,0x08,0x08,0x07},//6 6
    {0x00,0x10,0x10,0x10,0x90,0x70,0x00,0x00,0x00,0x0E,0x01,0x00},//7 7
    {0x00,0x60,0x90,0x90,0x90,0x60,0x00,0x07,0x08,0x08,0x08,0x07},//8 8
    {0x00,0xE0,0x10,0x10,0x10,0xE0,0x00,0x04,0x09,0x09,0x09,0x07},//9 9
};

static const u8 digit_6_16_e[][12] = {
    {0xFF,0x1F,0xEF,0xEF,0xEF,0x1F,0xFF,0xF8,0xF7,0xF7,0xF7,0xF8},//0 0
    {0xFF,0xEF,0xEF,0x0F,0xFF,0xFF,0xFF,0xF7,0xF7,0xF0,0xF7,0xF7},//1 1
    {0xFF,0xDF,0xEF,0xEF,0xEF,0x1F,0xFF,0xF7,0xF3,0xF5,0xF6,0xF7},//2 2
    {0xFF,0xEF,0x6F,0x6F,0x6F,0x9F,0xFF,0xF7,0xF7,0xF7,0xF7,0xF8},//3 3
    {0xFF,0x7F,0x9F,0xEF,0x0F,0xFF,0xFF,0xFC,0xFD,0xFD,0xF0,0xFD},//4 4
    {0xFF,0x0F,0x6F,0x6F,0x6F,0xFF,0xFF,0xF7,0xF7,0xF7,0xF7,0xF8},//5 5
    {0xFF,0x1F,0x6F,0x6F,0x6F,0xFF,0xFF,0xF8,0xF7,0xF7,0xF7,0xF8},//6 6
    {0xFF,0xEF,0xEF,0xEF,0x6F,0x8F,0xFF,0xFF,0xFF,0xF1,0xFE,0xFF},//7 7
    {0xFF,0x9F,0x6F,0x6F,0x6F,0x9F,0xFF,0xF8,0xF7,0xF7,0xF7,0xF8},//8 8
    {0xFF,0x1F,0xEF,0xEF,0xEF,0x1F,0xFF,0xFB,0xF6,0xF6,0xF6,0xF8},//9 9
};


static const u8 num_space_6x16[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//" "

static const u8 num_space_3x16[] = {0x00,0x00,0x00,0x00,0x00,0x00};//" "


static const u8 num_dot_6x16[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x00,0x00};//.
static const u8 num_star_6x16[] = {0x00,0x40,0x80,0xE0,0x80,0x40,0x00,0x05,0x03,0x0F,0x03,0x05};//*

static const u8 text_number_em_0000_6x16[] = {       // "!"
    0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x00,
};

static const u8 text_number_pc_0000_6x16[] = {   // "%"
    0x00,0x30,0x48,0xB0,0x60,0x18,0x00,0x08,0x06,0x01,0x06,0x09,
};

static const u8 text_number_co_0000_6x16[] = {   // ","
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x14,0x0C,0x00,
};

static const u8 text_number_bs_0000_6x16[] = {   // "/"
    0x00,0x00,0x00,0xE0,0x1C,0x00,0x00,0x38,0x07,0x00,0x00,0x00
};


static const u8 text_number_xx_0000_6x16[] = {   // "/"
    0x00,0x00,0x00,0xE0,0x1C,0x00,0x00,0x38,0x07,0x00,0x00,0x00
};

static const u8 text_number_colon_0000_6x16[] = {   // :
    0x00, 0x00, 0x60, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x06, 0x00, 0x00,
};

static const u8 text_number_colon_0000_6x16_e[] =  {    // :
    0xff, 0xff, 0x9f, 0x9f, 0xff, 0xff,
    0xff, 0xff, 0xf9, 0xf9, 0xff, 0xff,
};

static const u8 text_left_bracket_6x16[] = {   // "("
    0x00,0x00,0xE0,0x18,0x04,0x00,0x00,0x00,0x03,0x0C,0x10,0x00,
};

static const u8 text_right_bracket_6x16[] = {   // "("
    0x00,0x04,0x18,0xE0,0x00,0x00,0x00,0x10,0x0C,0x03,0x00,0x00,
};


//higligh
static const u8 num_space_6x16_e[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};//" " highlight
static const u8 num_space_3x16_e[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};   //" " highlight

static const u8 num_dot_6x16_e[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF3,0xFF,0xFF};//. highlight
static const u8 num_star_6x16_e[] = {0xFF,0xBF,0x7F,0x1F,0x7F,0xBF,0xFF,0xFA,0xFC,0xF0,0xFC,0xFA};// * highlight
static const u8 text_number_em_6x16_e[] = {0xFF,0xFF,0xFF,0x07,0xFF,0xFF,0xFF,0xFF,0xFF,0xF6,0xFF,0xFF};
static const u8 text_number_pc_6x16_e[] = {0xFF,0xCF,0xB7,0x4F,0x9F,0xE7,0xFF,0xF3,0xFC,0xF9,0xF6,0xF9};
static const u8 text_number_co_6x16_e[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEB,0xF3,0xFF};
static const u8 text_number_bs_6x16_e[] = {0xFF,0xFF,0xFF,0x1F,0xE3,0xFF,0xFF,0xC7,0xF8,0xFF,0xFF,0xFF};

static const u8 (*digit_array[])[12] = { digit_6_16, digit_6_16_e } ;

static CHAR_OTHER others_array[] = {
    {' ', {num_space_6x16, num_space_6x16_e}},
    {'.', {num_dot_6x16, num_dot_6x16_e}},
    {'*', {num_star_6x16, num_star_6x16_e}},
    {'!', {text_number_em_0000_6x16, text_number_em_6x16_e}},
    {'%', {text_number_pc_0000_6x16, text_number_pc_6x16_e}},
    {',', {text_number_co_0000_6x16, text_number_co_6x16_e}},
    {'/', {text_number_bs_0000_6x16, text_number_bs_6x16_e}},
    {':', {text_number_colon_0000_6x16, text_number_colon_0000_6x16_e}},
    {'(', {text_left_bracket_6x16, text_left_bracket_6x16}},
    {')', {text_right_bracket_6x16, text_right_bracket_6x16}},
};


#endif //FONT_ASCII_H
