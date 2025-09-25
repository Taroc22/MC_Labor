#include <msp430.h>

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;

    P1DIR |= BIT0;
    P1REN |= BIT1;  
    P1OUT |= BIT1;  

    P2DIR = 0x00; 
    P2REN |= BIT1;   
    P2OUT |= BIT1;   

    P4DIR |= BIT7;   

    unsigned int lastState1 = (P2IN & BIT1);
    unsigned int lastState2 = (P1IN & BIT1);

    while (1) {

        unsigned int nowP1 = (P2IN & BIT1);
        unsigned int nowP2 = (P1IN & BIT1);

        if (lastState1 && !nowP1) {
            P1OUT ^= BIT0; 
        }
        lastState1 = nowP1;

        if (lastState2 && !nowP2) {
            P4OUT ^= BIT7; 
        }
        lastState2 = nowP2;
    }
}
