#include <msp430.h>

void main(void)
{
    // Init 
    WDTCTL = WDTPW + WDTHOLD; // Watchdog aus!

    P1DIR = BIT0;    // P1.0 als Ausgang (LED1)
    P1REN = BIT1;    // P1.1 Pull Resistor
    P1OUT = BIT1;    // P1.1 Pull-Up aktivieren

    P2DIR = 0x00;    // P2.x Eingänge
    P2REN = BIT1;    // P2.1 Pull Resistor
    P2OUT = BIT1;    // P2.1 Pull-Up aktivieren

    P4DIR = BIT7;    // P4.7 als Ausgang (LED2)

    unsigned int lastState1 = (P2IN & BIT1);
    unsigned int lastState2 = (P1IN & BIT1);

    while (1) {

        unsigned int nowP1 = (P2IN & BIT1);
        unsigned int nowP2 = (P1IN & BIT1);

        // Flanke High -> Low (Taste gedrückt wegen PullUp)
        if (lastState1 && !nowP1) {
            P1OUT ^= BIT0;   // LED1 toggeln
        }
        lastState1 = nowP1;

        if (lastState2 && !nowP2) {
            P4OUT ^= BIT7;   // LED2 toggeln
        }
        lastState2 = nowP2;
    }
}