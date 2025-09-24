#include <msp430.h>

void main(void){

    // WDTCTL = Watchdog Timer Control
    // WDTPW = Watchdog Timer Password
    // WDTCNTCL = Watchdog Counter Clear
    // WDTSSEL_1 = Watchdog Source Select 1(ACLK)
    // WDTIS_4 = Watchdog Timer Intervall Select 4(ca.1s)
	WDTCTL = WDTPW + WDTCNTCL + WDTSSEL_1 + WDTIS_4;

	P1DIR |= BIT0;    // P1.0 als Ausgang (LED1)
    P1REN |= BIT1;    // P1.1 Pull Resistor
    P1OUT |= BIT1;    // P1.1 Pull-Up aktivieren

    P2DIR = 0x00;     // P2.x Eingänge (Eigentlich überflüssig)
    P2REN |= BIT1;    // P2.1 Pull Resistor
    P2OUT |= BIT1;    // P2.1 Pull-Up aktivieren


    unsigned int lastState1 = (P2IN & BIT1);
    unsigned int lastState2 = (P1IN & BIT1);

	while(1){
        // Watchdog zurücksetzen/ neustarten
        WDTCTL = WDTPW + WDTCNTCL + WDTSSEL_1 + WDTIS_4;

		unsigned int nowP1 = (P2IN & BIT1);
        unsigned int nowP2 = (P1IN & BIT1);

        // Flanke High -> Low (Taste gedrückt wegen PullUp)
        if (lastState1 && !nowP1) {
            P1OUT ^= BIT0;   // LED1 toggeln
        }
        lastState1 = nowP1;

        if (lastState2 && !nowP2) {
            while(1){
				//Endlosschleife
			}
        }
        lastState2 = nowP2;
		
	}
}
