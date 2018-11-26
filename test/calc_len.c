#include <stdio.h>
#include <string.h>


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

typedef struct {
    char ch;
    int  iLen;
} CHAR_INFO;

static CHAR_INFO gCharTab[] = {

   { 'a', 6}, // a 65
   { 'b', 6}, // b 66
   { 'c', 5}, // c 67
   { 'd', 6}, // d 68
   { 'e', 6}, // e 69
   { 'f', 4}, // f 70
   { 'g', 6}, // g 71
   { 'h', 6}, // h 72
   { 'i', 2}, // i 73
   { 'j', 3}, // j 74
   { 'k', 6}, // k 75
   { 'l', 2}, // l 76
   { 'm', 8}, // m 77
   { 'n', 6}, // n 78
   { 'o', 6}, // o 79
   { 'p', 6}, // p 80
   { 'q', 6}, // q 81
   { 'r', 4}, // r 82
   { 's', 6}, // s 83
   { 't', 4}, // t 84
   { 'u', 6}, // u 85
   { 'v', 6}, // v 86
   { 'w', 8}, // w 87
   { 'x', 6}, // x 88
   { 'y', 6}, // y 89
   { 'z', 6}, // z 90     

    { 'A', 8}, // A 33
    { 'B', 7}, // B 34
    { 'C', 7}, // C 35
    { 'D', 8}, // D 36
    { 'E', 6}, // E 37
    { 'F', 6}, // F 38
    { 'G', 7}, // G 39
    { 'H', 7}, // H 40
    { 'I', 4}, // I 41
    { 'J', 4}, // J 42
    { 'K', 6}, // K 43
    { 'L', 6}, // L 44
    { 'M', 8}, // M 45
    { 'N', 7}, // N 46
    { 'O', 8}, // O 47
    { 'P', 6}, // P 48
    { 'Q', 8}, // Q 49
    { 'R', 8}, // R 50
    { 'S', 6}, // S 51
    { 'T', 8}, // T 52
    { 'U', 8}, // U 53
    { 'V', 8}, // V 54
    { 'W', 10}, // W 55
    { 'X', 7}, // X 56
    { 'Y', 8}, // Y 57
    { 'Z', 8}, // Z 58   

    {'0', 6},
    {'1', 6},
    {'2', 6},
    {'3', 6},
    {'4', 6},
    {'5', 6},
    {'6', 6},
    {'7', 6},
    {'8', 6},
    {'9', 6},

    {' ', 6},
    {'.', 6},
    {'*', 6},
    {'!', 6},
    {'%', 6},
    {',', 6},
    {'/', 6},
    {':', 6},
    {'(', 6},
    {')', 6},
};


/*
 * 数字
 */

/*
 * 特殊字符
 */






int main(int argc, char**argv) 
{
    int i = 0, j = 0;
    char c;
    int iTotalLen = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: calc_len <\"Test String\">\n");
        return -1;
    }

    printf("Calc String: %s\n", argv[1]);

    for (i = 0; i < strlen(argv[1]); i++) {
        c = argv[1][i];
        printf("c = %c\n", c);

        for (j = 0; j < ARRAY_SIZE(gCharTab); j++) {
            if (c == gCharTab[j].ch) {
                iTotalLen += gCharTab[j].iLen;
                break;
            } 
        }

        if (j >= ARRAY_SIZE(gCharTab)) {
            printf("char  [%c] not in gCharTab, use default val 6", c);
            iTotalLen += 6;
        }
    }

    printf("String [%s] total len [%d]\n", argv[1], iTotalLen);   
    printf("String start pos [%d]\n", (128 - iTotalLen) / 2 );   

    return 0;
}