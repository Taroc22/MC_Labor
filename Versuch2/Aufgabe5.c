#include <msp430.h>

#define TIMER_INTERVAL 0xFFF
#define CTL_STEP 30

void main(void){

    //Buzzer ist angeschlossen an P2.5 entspricht TA2.2 (Timer A2 CCR2)

    WDTCTL = WDTPW + WDTHOLD;

    P1REN |= BIT1;    // P1.1 Pull Resistor
    P1OUT |= BIT1;    // P1.1 Pull-Up aktivieren

    P2REN |= BIT1;    // P2.1 Pull Resistor
    P2OUT |= BIT1;    // P2.1 Pull-Up aktivieren

    // TA2CTL = Timer A2 Control Register
    // TASSEL1 = Timer A Source Select 1(ACLK)
    // MC_1 = Mode Control 1 (Up Mode until value of TA2CCR0)
    // ID_0 = Timer A input divider: 0 - /1 */
    TA2CTL = TASSEL1 + MC_1 + ID_0;
    TA2CCR0 = TIMER_INTERVAL; // Set Timer Interval

    TA2CCR2 = TIMER_INTERVAL / 2; //Initialisieren von Buzzer auf halbe Lautstärke
    TA2CCTL2 = OUTMOD_3; // Timer A2 Capture Compare Control Register auf PWM set/reset setzen

    P2DIR |= BIT5; // Pin als Ausgang festlegen    
    P2SEL |= BIT5; // Peripherie Mode des Buzzer Pins        
    
    while(1){
        if (!(P1IN & BIT1)) {    // Invert wegen PullUp
            if (TA2CCR2 >= CTL_STEP) {  // prüfen, dass nicht < 0
                TA2CCR2 -= CTL_STEP;
            } else {
                TA2CCR2 = 0;      // untere Grenze
            }
            for(int i=0; i<2000; i++); // Delay um Sprung von 0 auf 100% zu vermeiden
        }

        if (!(P2IN & BIT1)) {	// Invert wegen PullUp
            if (TA2CCR2 <= (TIMER_INTERVAL - CTL_STEP)) {  // prüfen, dass nicht > MAX
                TA2CCR2 += CTL_STEP;
            } else {
                TA2CCR2 = TIMER_INTERVAL;  // obere Grenze
            }
            for(int i=0; i<2000; i++); // Delay um Sprung von 0 auf 100% zu vermeiden
        }
    }

}
