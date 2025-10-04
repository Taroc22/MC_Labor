/*
    Snake game for MSP430 with BOOSTXL-EDUMKII BoosterPack
    @author: Amir Tannouri
*/

#include <msp430.h>
#include "ST7735.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define sb(reg, bit) ((reg) |= (bit))
#define cb(reg, bit) ((reg) &= ~(bit))
#define tb(reg, bit) ((reg) ^= (bit))

//0xBBGGRR
#define WHITE   0xFFFFFF
#define BLACK   0x000000
#define WHITE   0xFFFFFF
#define RED     0x0000FF
#define GREEN   0x00FF00
#define BLUE    0x0000FF
#define YELLOW  0xFFFF00
#define BG      0xD40D48

#define AT "Amir Tannouri"
#define BY "by"
#define NM "SNAKE"
#define PR "PRESS"
#define ST "START"

#define FONT_WIDTH      7
#define FONT_HEIGHT     12
#define CHAR_SPACING    0
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  128

#define ROWS            16
#define COLS            16
#define SNAKE_WIDTH     8
#define SNAKE_HEIGHT    8

#define ADC_MAX         4095
#define ADC_CENTER      (ADC_MAX / 2)
#define DEADZONE        200

#define CLK             4096/1000 * 200

/*
    Anzahl Zeilen (Höhe): 0-10
    Displaygröße: 128x128 (y wird aber gestreckt)

    Timer A0 = delay

    Feldgröße (Pixel)	Felder pro Achse
        8×8	                16×16
*/

volatile uint8_t tick = 0;

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

typedef enum {
    CENTER,
    UP,
    DOWN,
    LEFT,
    RIGHT
} Dir;

volatile Dir curDir = CENTER;

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
    { REG_BIT, &P2SEL, BIT5 }, //Buzzer
    { REG_VAL, &TA1CCTL0, CCIE },   //Timerconfig evtl auslagern in Hauptroutine
    { REG_VAL, &TA1CCR0, CLK },
    { REG_VAL, &TA1CTL, TASSEL_1|MC_1|ID_3|TACLR },
    { REG_VAL, &ADC12CTL0, ADC12SHT0_8 },   //Sample-and-Hold 256 ADC-Takte
    { REG_VAL, &ADC12CTL1, ADC12SHP },      //Sampling Timer verwenden
    { REG_VAL, &ADC12CTL2, ADC12RES_2 },    //12-bit Auflösung
    { REG_VAL, &ADC12CTL0, ADC12ON | ADC12ENC } //ADC einschalten und aktivieren
};


void delay(unsigned int ms){
    TA0CCR0 = 4096/1000 * ms;
    TA0CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;
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
    __bis_SR_register(GIE);
}


unsigned int readADC(unsigned int channel)
{
    ADC12CTL0 &= ~ADC12ENC;       // Konversion deaktivieren um Channel zu wechseln
    ADC12MCTL0 = channel;         // ADC-Memory0 auf Channel einstellen (z. B. A5 = 5)
    ADC12CTL0 |= ADC12ENC;        // ADC aktivieren

    ADC12CTL0 |= ADC12SC;         // Konversion starten: Sample-and-Hold + A/D-Wandlung
    while (ADC12CTL1 & ADC12BUSY); // Warten bis ADC fertig ist (BUSY=0 ist fertig)

    return ADC12MEM0;             // return Messwert (0–4095)
}


PixelPos gridToPixel(GridPos g) {
    PixelPos p;
    p.x = g.col * SNAKE_WIDTH;
    p.y = g.row * SNAKE_HEIGHT;
    return p;
}


Dir scaleADC(uint16_t x, uint16_t y) {
    int16_t dx = (int16_t)x - ADC_CENTER;
    int16_t dy = (int16_t)y - ADC_CENTER;

    if (abs(dx) < DEADZONE && abs(dy) < DEADZONE) {
        return CENTER;   
    }

    if (abs(dx) > abs(dy)) {
        return (dx < 0) ? LEFT : RIGHT;
    } else {
        return (dy < 0) ? DOWN : UP;
    }
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


unsigned int checkCollision(Dir dir){
    //Check if move dir from currPos in field possible
    //return 1 if yes else 0
}


//Auf ganzen Snake Körper erweitern
void drawSnake(){
    PixelPos p = gridToPixel(currPos);
    draw(p.x, p.y, SNAKE_WIDTH, SNAKE_HEIGHT, RED);
}


void main(){
    setup();
    start();

    currPos = (GridPos){8, 8};
    field[currPos.row][currPos.col] = 1;
    unsigned int joyX, joyY;
    curDir = (Dir)((rand() % 4) + 1); //init dir randomly

    drawSnake();

    //debug
    //char buffer[32];  
    //const char* dirNames[] = { "CENTER", "UP", "DOWN", "LEFT", "RIGHT" }; 

    while(1){
        if (tick == 1){

            //read ADC
            joyX = readADC(5);   // P6.5
            joyY = readADC(3);   // P6.3

            //scale ADC
            curDir = scaleADC(joyX, joyY);

            //debug
            //sprintf(buffer, "X:%4u Y:%4u", joyX, joyY);
            //drawTextLine(2, 0, buffer, GREEN, BLACK);
            //sprintf(buffer, "Dir=%s", dirNames[curDir]);
            //drawTextLine(6, 0, buffer, GREEN, BLACK);

            //check if next pos is free/ collision
                //if yes: redraw snake to new position/ update field variables (currPos, field)
                //if no: game over

            tick = 0;
        }
    }
}


// Timer1_A0 ISR => wird bei CCR0 erreicht ausgelöst
__attribute__((interrupt(TIMER1_A0_VECTOR)))
void TIMER1_A0_ISR(void) {
    tick = 1;
    sb(TA1CTL, TACLR);
    TA1CCTL0 &= ~CCIFG; 
}