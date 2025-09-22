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

    while (1) {
        if (!(P1IN & BIT1)) {   // Invert wegen PullUp
            P1OUT |= BIT0;      // LED an
        } else {
            P1OUT &= ~BIT0;     // LED aus
        }

        if (!(P2IN & BIT1)) {	// Invert wegen PullUp
            P4OUT |= BIT7;      // LED an
        } else {
            P4OUT &= ~BIT7;     // LED aus
        }
    }
}
