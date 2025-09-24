#include <msp430.h>

#define TIMER_INTERVAL 0xFFF

void main(void){

    //LCD Beleuchtung ist angeschlossen an P2.4 entspricht TA2.1 (Timer A2 internes binäres Signal 1)

    WDTCTL = WDTPW + WDTHOLD;

    P1REN |= BIT1;    // P1.1 Pull Resistor
    P1OUT |= BIT1;    // P1.1 Pull-Up aktivieren

    P2REN |= BIT1;    // P2.1 Pull Resistor
    P2OUT |= BIT1;    // P2.1 Pull-Up aktivieren

    // TA0CTL = Timer A0 Control Register
    // TASSEL1 = Timer A Source Select 1(ACLK)
    // MC_1 = Mode Control 1 (Up Mode until value of TA2CCR0)
    // ID_0 = Timer A input divider: 0 - /1 */
    TA2CTL = TASSEL1 + MC_1 + ID_0;
    TA2CCR0 = TIMER_INTERVAL; // Set Timer Interval

    TA2CCR1 = TIMER_INTERVAL / 2; //Initialisieren von LCD auf halbe Helligkeit
    TA2CCTL1 = OUTMOD_3; // Timer A2 Capture Compare Control Register auf PWM set/reset setzen

    P2DIR |= BIT4; // Pin als Ausgang festlegen    
    P2SEL |= BIT4; // Peripherie Mode des Pins        
    
    while(1){
        if (!(P1IN & BIT1)) {    // Invert wegen PullUp
            if (TA2CCR1 >= 50) {  // prüfen, dass nicht < 0
                TA2CCR1 -= 50;
            } else {
                TA2CCR1 = 0;      // untere Grenze
            }
            for(int i=0; i<2000; i++); // Delay um Sprung von 0 auf 100% zu vermeiden
        }

        if (!(P2IN & BIT1)) {	// Invert wegen PullUp
            if (TA2CCR1 <= (TIMER_INTERVAL - 50)) {  // prüfen, dass nicht > MAX
                TA2CCR1 += 50;
            } else {
                TA2CCR1 = TIMER_INTERVAL;  // obere Grenze
            }
            for(int i=0; i<2000; i++); // Delay um Sprung von 0 auf 100% zu vermeiden
        }
    }

}
