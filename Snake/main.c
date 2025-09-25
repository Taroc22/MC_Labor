#include <msp430.h>
#include "ST7735.h"
#include <string.h>

#define WHITE 0xFFFFFF
#define BG 0xd40d48
#define AT "Amir Tannouri"
#define BY "by"
#define NM "SNAKE"

#define FONT_WIDTH 7
#define FONT_HEIGHT 12
#define CHAR_SPACING 0
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128


/*
    Anzahl Zeilen (Höhe): 0-10
    Displaygröße: 128x128 (y wird aber gestreckt)
*/

void delay(unsigned int ms){
    TA0CCR0 = 4096/1000 * ms;
    TA0CTL =TASSEL_1 + MC_1 + ID_3 + TACLR;
    while (!(TA0CCTL0 & CCIFG));
    TA0CTL = 0;                      
    TA0CCTL0 &= ~CCIFG;   
}

int centerText(const char *text) {
    int len = strlen(text);
    int width = len * FONT_WIDTH + (len - 1) * CHAR_SPACING;
    return (DISPLAY_WIDTH - width) / 2;
}


void setup(){
    WDTCTL = WDTPW + WDTHOLD;
    ST7735_interface_init();  
    ST7735_display_init();
    
    draw(0, 0, 128, 128, BG);
    delay(2000);
    setText(centerText(NM), 25, NM, WHITE, BG);
    delay(2000);
    setText(centerText(BY), 47, BY, WHITE, BG);
    setText(centerText(AT), 69, AT, WHITE, BG);
    delay(2000);
    draw(0, 0, 128, 128, BG);
}


void main(){
    setup();
}