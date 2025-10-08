#include <msp430.h>
#include <stdio.h>
#include "ST7735.h"

#define COLOR_WHITE  0xFFFFFF
#define COLOR_BLACK  0x000000


void printAllChars(void)
{
    unsigned char line = 0;
    unsigned char pos  = 0;
    char buffer[19];
    buffer[18] = '\0';

    unsigned char ch = 0x7F;

    while (ch <= 0xFF && line <= 7)
    {
        for (int i = 0; i < 18; i++)
        {
            if (ch <= 0xFF)
                buffer[i] = ch++;
            else
                buffer[i] = ' ';
        }

        drawTextLine(line, pos, buffer, COLOR_WHITE, COLOR_BLACK);
        line++;
    }
}


void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;

    ST7735_interface_init();
    ST7735_display_init();

    printAllChars();
}
