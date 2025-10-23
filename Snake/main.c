/*
    Snake game for MSP430 with BOOSTXL-EDUMKII BoosterPack
    @author: Amir Tannouri | 2025
*/

#include <msp430.h>
#include "ST7735.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "melody.h"

#define sb(reg, bit) ((reg) |= (bit))
#define cb(reg, bit) ((reg) &= ~(bit))
#define tb(reg, bit) ((reg) ^= (bit))

//0xBBGGRR
#define BLACK   0x000000
#define WHITE   0xFFFFFF
#define RED     0x0000FF
#define GREEN   0x00FF00
#define BLUE    0xFF0000
#define YELLOW  0x00FFFF
#define BG      0xD40D48

#define AT "Amir Tannouri"
#define BY "by"
#define NM "SNAKE"
#define PR "PRESS"
#define ST "START"
#define GO "GAME OVER"

#define FONT_WIDTH      7
#define FONT_HEIGHT     12
#define CHAR_SPACING    0
#define DISPLAY_WIDTH   128
#define DISPLAY_HEIGHT  128

#define ROWS            16
#define COLS            16
#define SNAKE_WIDTH     8
#define SNAKE_HEIGHT    8
#define MAX_SNAKE_LENGTH (ROWS * COLS)

#define ADC_MAX         4095
#define ADC_CENTER      (ADC_MAX / 2)
#define DEADZONE        500

#define CLK             4096/1000 * 500 //clock for main loop; adjust for game speed

#define MELODY_LEN (sizeof(melody)/sizeof(melody[0]))

/*
    Anzahl Zeilen (Höhe): 0-10
    Displaygröße: 128x128 (y wird aber gestreckt)

    Feldgröße (Pixel)	Felder pro Achse
        8×8	                16×16      
    
    Timer A0 = delay
    Timer A1 = tick for main loop
    Timer A2 = PWM Signal for Buzzer
    Timer B0 = non blocking trigger for Buzzer off

    To eliminate space between snake body parts remove addition/subtraction 
    from draw() inside drawSnake() and spawnFood()
*/

void initFlash(void);
unsigned int joyX, joyY;
uint8_t score = 0;
volatile uint8_t tick = 0;

uint8_t field[ROWS][COLS];

typedef struct {
    uint8_t row;
    uint8_t col;
} GridPos;

GridPos currPos; //max 0-15 for row&col

GridPos snake[MAX_SNAKE_LENGTH];
uint16_t snakeLength = 1;

typedef struct {
    uint8_t x;
    uint8_t y;
} PixelPos; 

typedef enum {
    CENTER,
    UP,
    DOWN,
    LEFT,
    RIGHT
} Dir;

volatile Dir currDir = CENTER;

enum RegType {REG_BIT, REG_VAL};

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
    { REG_BIT, &P2DIR, BIT5 },                  //Buzzer
    { REG_BIT, &P2SEL, BIT5 },                  //Buzzer
    { REG_VAL, &TB0CCTL0, CCIE },                
    { REG_VAL, &TA1CCTL0, CCIE },
    { REG_VAL, &TA1CCR0, CLK },
    { REG_VAL, &TA1CTL, TASSEL_1|MC_1|ID_3|TACLR },
    { REG_VAL, &ADC12CTL0, ADC12SHT0_8 },       //Sample-and-Hold 256 ADC-Cycles
    { REG_VAL, &ADC12CTL1, ADC12SHP },          //Use Sampling Timer
    { REG_VAL, &ADC12CTL2, ADC12RES_2 },        //12-bit resolution
    { REG_VAL, &ADC12CTL0, ADC12ON|ADC12ENC }   //ADC on and enable
};

volatile uint8_t __attribute__((section(".infoD"))) highscore;


void delay(unsigned int ms) {
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


void initMCU() {
    WDTCTL = WDTPW + WDTHOLD;
    for (int i = 0; i < sizeof(ops)/sizeof(ops[0]); i++) {
        if (ops[i].type == REG_BIT) {
            sb(*(volatile unsigned char*)ops[i].reg, (unsigned char)ops[i].value);
        } else if (ops[i].type == REG_VAL) {
            *(volatile unsigned int*)ops[i].reg = ops[i].value;
        }
    }
    initFlash();
    __bis_SR_register(GIE);
    ST7735_interface_init();  
    ST7735_display_init();
}


unsigned int readADC(unsigned int channel) {
    ADC12CTL0 &= ~ADC12ENC;       // disable conversion to change channel
    ADC12MCTL0 = channel;         // ADC-Memory0 set channel  (e.g. A5 = 5)
    ADC12CTL0 |= ADC12ENC;        // ADC enable

    ADC12CTL0 |= ADC12SC;         // start conversion: Sample-and-Hold + A/D-Conversion
    while (ADC12CTL1 & ADC12BUSY);// Wait until ADC reading finished (BUSY=0 -> finished)

    return ADC12MEM0;             // return adc value (0–4095)
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


void buzzer_on(unsigned int frequency) {
    unsigned int period = 32768 / frequency;
    if (period == 0) period = 1;

    TA2CCR0 = period - 1;       // period duration
    TA2CCR2 = period / 2;       // 50% Duty Cycle
    TA2CCTL2 = OUTMOD_3;
    TA2CTL = TASSEL_1 + MC_1 + ID_0;
}


void buzzer_off(void) {
    TA2CTL = MC_0;
    P2OUT &= ~BIT5;
}


void play_tone(unsigned int freq, unsigned int duration_ms) {
    buzzer_on(freq);
    TB0CTL = MC_0;
    TB0CCTL0 &= ~CCIFG;
    TB0CCR0 = 4096/1000 * duration_ms;
    TB0CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;
}


volatile uint8_t melody_playing = 0;
volatile uint8_t melody_index = 0;


void start_melody(void) {
    melody_playing = 1;
    melody_index = 0;
    play_tone(melody[0].freq, melody[0].duration);
}


void stop_melody(void) {
    melody_playing = 0;
    buzzer_off();
    TB0CTL = MC_0;
}


void initFlash(void) {
    FCTL3 = FWKEY + LOCK; //lock Flash
}


uint8_t readFlash(void) {
    return highscore;
}


void eraseFlash(void) {
    uint8_t *ptr = (uint8_t *)&highscore;

    __disable_interrupt(); 
    FCTL3 = FWKEY;             //unlock
    FCTL1 = FWKEY + ERASE;     //erase mode
    *ptr = 0;                  //dummy write starts erasing

    while (FCTL3 & BUSY);      //wait until done

    FCTL1 = FWKEY;             //end
    FCTL3 = FWKEY + LOCK;
    __enable_interrupt();
}


void writeFlash(uint8_t newScore) {
    uint8_t *ptr = (uint8_t *)&highscore;

    __disable_interrupt(); 
    eraseFlash();              //erase segment

    FCTL3 = FWKEY;             //unlock
    FCTL1 = FWKEY + WRT;       //write mode
    *ptr = newScore;           //write byte

    FCTL1 = FWKEY;             //end
    FCTL3 = FWKEY + LOCK;
    __enable_interrupt();
}


//debug
void clearFlash() {
    writeFlash(0x00);
}


void setup() {
    initMCU();
    
    draw(0, 0, 128, 128, BG);
    delay(2000);
    setText(centerText(NM), 30, NM, WHITE, BG);
    delay(2000);
    setText(centerText(BY), 52, BY, WHITE, BG);
    setText(centerText(AT), 74, AT, WHITE, BG);
    delay(2000);
    draw(0, 0, 128, 128, BG);
    delay(500);  
}


void start() { 
    //clearFlash(); //debug

    sb(P1OUT, BIT0);
    cb(P4OUT, BIT7);

    uint8_t hs = readFlash();
    char hsText[32]; 
    sprintf(hsText, "Highscore: %u", hs);

    while (!(P1IFG & BIT1) && !(P2IFG & BIT1)) {
        setText(centerText(PR), 20, PR, WHITE, BG);
        setText(centerText(ST), 55, ST, WHITE, BG);
        setText(centerText(hsText), 85, hsText, WHITE, BG);
        tb(P1OUT, BIT0);
        tb(P4OUT, BIT7);
        delay(600);
        setText(centerText(PR), 20, PR, BG, BG);
        setText(centerText(ST), 55, ST, BG, BG);
        setText(centerText(hsText), 85, hsText, BG, BG);
        tb(P1OUT, BIT0);
        tb(P4OUT, BIT7);
        delay(600);
    }
    draw(0, 0, 128, 128, BLACK);
    if (P1IFG & BIT1) P1IFG &= ~BIT1;
    if (P2IFG & BIT1) P2IFG &= ~BIT1;
}


PixelPos gridToPixel(GridPos g) {
    PixelPos p;
    p.x = g.col * SNAKE_WIDTH;
    p.y = g.row * SNAKE_HEIGHT;
    return p;
}


// returns 0 if collison, 1 if none
unsigned int checkCollision(Dir *lastDir) {
    // If currDir == CENTER keep lastDir
    Dir dirToCheck = (currDir == CENTER) ? *lastDir : currDir;

    int newRow = currPos.row;
    int newCol = currPos.col;

    switch(dirToCheck) {
        case UP:    newRow--; break;
        case DOWN:  newRow++; break;
        case LEFT:  newCol--; break;
        case RIGHT: newCol++; break;
        default: break;
    }

    if(newRow < 0 || newRow >= ROWS || newCol < 0 || newCol >= COLS)
        return 0; //wall collision

    if(field[newRow][newCol] == 1)
        return 0; //body collision
        
    *lastDir = dirToCheck;
    currPos = (GridPos){newRow, newCol};
    return 1;
}


void drawSnake() {
    PixelPos p = gridToPixel(snake[0]);
    draw(p.x+1, p.y+1, SNAKE_WIDTH-2, SNAKE_HEIGHT-2, GREEN);
}


void spawnFood(void) {
    GridPos freeCells[ROWS * COLS];
    unsigned int freeCount = 0;

    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < COLS; c++) {
            if (field[r][c] == 0) {
                freeCells[freeCount++] = (GridPos){r, c};
            }
        }
    }

    if (freeCount == 0) return; //implement game won here

    unsigned int index = rand() % freeCount;
    GridPos foodPos = freeCells[index];

    field[foodPos.row][foodPos.col] = 2;
    PixelPos p = gridToPixel(foodPos);
    draw(p.x+1, p.y+1, SNAKE_WIDTH-2, SNAKE_HEIGHT-2, RED);
}


void checkFood() {
    if(field[currPos.row][currPos.col] == 2) {
        score++;
        //play_tone(10000, 100);   // 10 kHz for 100ms; blocking
        spawnFood();

        //push snake[] one index up and add currPos at index 0
        for (int i = snakeLength; i > 0; i--) {
            snake[i] = snake[i-1];
        }
        snake[0] = currPos;
        snakeLength++;

    } else {
        //always delete last element of snake[]
        GridPos last = snake[snakeLength - 1];
        PixelPos p = gridToPixel(last);
        draw(p.x, p.y, SNAKE_WIDTH, SNAKE_HEIGHT, BLACK);
        field[last.row][last.col] = 0;
        
        //push snake[] one index up, delete last element and add currPos at index 0
        for (int i = snakeLength - 1; i > 0; i--) {
            snake[i] = snake[i-1];
        }
        snake[0] = currPos;
    }
    field[currPos.row][currPos.col] = 1;
}


void main() {
    start_melody();
    setup();
    restart:
    start();
    srand(TA1R); //init seed randomly

    for (int i = 0; i < ROWS; i++){
        for (int j = 0; j < COLS; j++){
            field[i][j] = 0;
        }
    }

    currPos = (GridPos){8, 8};
    field[currPos.row][currPos.col] = 1;
    snake[0] = currPos;
    //init lastDir randomly in case first adc reading is CENTER
    Dir lastDir = (Dir)((rand() % 4) + 1); // exclude CENTER
    drawSnake();
    spawnFood();

    while(1){
        if (tick == 1){

            joyX = readADC(5);   // P6.5
            joyY = readADC(3);   // P6.3

            currDir = scaleADC(joyX, joyY);
            
            if(checkCollision(&lastDir) == 1) {
                checkFood();
                drawSnake();
            } else {
                if (score > readFlash()){
                    draw(0, 0, 128, 128, BG);
                    setText(centerText(GO), 43, GO, WHITE, BG);
                    char sText[32]; 
                    sprintf(sText, "New Highscore: %u", score);
                    setText(centerText(sText), 70, sText, WHITE, BG);
                    break;
                }else {
                    draw(0, 0, 128, 128, BG);
                    setText(centerText(GO), 43, GO, WHITE, BG);
                    char sText[32]; 
                    sprintf(sText, "Score: %u", score);
                    setText(centerText(sText), 70, sText, WHITE, BG);
                    break;
                }
                
            }

            tick = 0;
        }
    }
    
    if (score > readFlash()){
        writeFlash(score);
    }

    delay(5000);
    draw(0, 0, 128, 128, BG);
    score = 0;
    snakeLength = 1;
    currDir = CENTER;
    goto restart;
}


// Timer1_A0 ISR => fires at CCR0
// runs the main loop
__attribute__((interrupt(TIMER1_A0_VECTOR)))
void TIMER1_A0_ISR(void) {
    tick = 1;
    sb(TA1CTL, TACLR);
    TA1CCTL0 &= ~CCIFG; 
}


// Timer0_B0 ISR => fires at CCR0
// triggers buzzer_off()
__attribute__((interrupt(TIMER0_B0_VECTOR)))
void TIMER0_B0_ISR(void) {
    buzzer_off();                 
    TB0CTL = MC_0;                
    TB0CCTL0 &= ~CCIFG;           

    if (melody_playing) {
        melody_index++;
        if (melody_index < MELODY_LEN) {
            play_tone(melody[melody_index].freq, melody[melody_index].duration);
        } else {
            melody_index = 0;
            play_tone(melody[melody_index].freq, melody[melody_index].duration);
        }
    }
}

