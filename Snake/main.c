/*
    Snake game for MSP430 with BOOSTXL-EDUMKII BoosterPack
    @author: Amir Tannouri
*/

#include <msp430.h>
#include "ST7735.h"
#include <string.h>
#include <stdint.h>

#define sb(reg, bit) ((reg) |= (bit))
#define cb(reg, bit) ((reg) &= ~(bit))
#define tb(reg, bit) ((reg) ^= (bit))

#define WHITE 0xFFFFFF
#define BG 0xd40d48
#define AT "Amir Tannouri"
#define BY "by"
#define NM "SNAKE"
#define PR "PRESS"
#define ST "START"

#define FONT_WIDTH 7
#define FONT_HEIGHT 12
#define CHAR_SPACING 0
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 128

#define ROWS 8
#define COLS 8

/*
    Anzahl Zeilen (Höhe): 0-10
    Displaygröße: 128x128 (y wird aber gestreckt)

    Timer A0 = delay

    Feldgröße (Pixel)	Felder pro Achse
        8×8	                16×16
*/

uint8_t field[ROWS][COLS];

typedef struct {
    uint8_t row;
    uint8_t col;
} GridPos;

typedef struct {
    uint8_t x;
    uint8_t y;
} PixelPos; 

GridPos currPos; //max 0-15 für row&col

enum RegType { REG_BIT, REG_VAL };

struct RegOp {
    enum RegType type;
    volatile void *reg;
    unsigned int value;
};

struct RegOp ops[] = {
    { REG_BIT, &P1REN, BIT1 },
    { REG_BIT, &P1OUT, BIT1 },
    { REG_BIT, &P2REN, BIT1 },
    { REG_BIT, &P2OUT, BIT1 },
    { REG_BIT, &P1DIR, BIT0 },
    { REG_BIT, &P4DIR, BIT7 },
    { REG_BIT, &P2DIR, BIT5 }, //Buzzer
    { REG_BIT, &P2SEL, BIT5 }  //Buzzer
    //ADC
    //{ REG_VAL, &TA0CTL, TASSEL_1 + MC_1 + ID_3 + TACLR } Beispiel
};


void delay(unsigned int ms){
    TA0CCR0 = 4096/1000 * ms;
    TA0CTL =TASSEL_1 + MC_1 + ID_3 + TACLR;
    while (!(TA0CCTL0 & CCIFG));
    TA0CTL = 0; 
    cb(TA0CCTL0, CCIFG);
}


int centerText(const char *text) {
    int len = strlen(text);
    int width = len * FONT_WIDTH + (len - 1) * CHAR_SPACING;
    return (DISPLAY_WIDTH - width) / 2;
}


void initMCU(){
    WDTCTL = WDTPW + WDTHOLD;
    for (int i = 0; i < sizeof(ops)/sizeof(ops[0]); i++) {
        if (ops[i].type == REG_BIT) {
            sb(*(volatile unsigned char*)ops[i].reg, (unsigned char)ops[i].value);
        } else if (ops[i].type == REG_VAL) {
            *(volatile unsigned int*)ops[i].reg = ops[i].value;
        }
    }
    for (int i = 0; i < ROWS; i++){
        for (int j = 0; j < COLS; j++){
            field[i][j] = 0;
        }
    }
}


PixelPos gridToPixel(GridPos g) {
    PixelPos p;
    p.x = g.col * COLS;
    p.y = g.row * ROWS;
    return p;
}


void setup(){
    initMCU();
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
    delay(500);  
}


void start(){    
    sb(P1OUT, BIT0);
    setText(centerText(PR), 35, PR, WHITE, BG);
    setText(centerText(ST), 70, ST, WHITE, BG);
    goto start;
    while (!(P1IFG & BIT1) && !(P2IFG & BIT1)){
        setText(centerText(PR), 35, PR, WHITE, BG);
        setText(centerText(ST), 70, ST, WHITE, BG);
        tb(P1OUT, BIT0);
        tb(P4OUT, BIT7);
        //Hier Buzzer anschalten
    start:
        delay(600);
        setText(centerText(PR), 35, PR, BG, BG);
        setText(centerText(ST), 70, ST, BG, BG);
        tb(P1OUT, BIT0);
        tb(P4OUT, BIT7);
        //Hier Buzzer ausschalten
        delay(600);
    }
    draw(0, 0, 128, 128, BG);
    if (P1IFG & BIT1) P1IFG &= ~BIT1;
    if (P2IFG & BIT1) P2IFG &= ~BIT1;
}


void main(){
    setup();
    start();
    currPos = (GridPos){8, 8};
    field[currPos.row][currPos.col] = 1;
}