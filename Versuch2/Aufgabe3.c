#include <msp430.h>

void main(void){

    //LCD Beleuchtung ist angeschlossen an P2.4 entspricht TA2.1 (Timer A2 internes binäres Signal 1)

    WDTCTL = WDTPW + WDTHOLD;

    // TA0CTL = Timer A0 Control Register
    // TASSEL1 = Timer A Source Select 1(ACLK)
    // MC_2 = Mode Control 2 (Continuous Mode)
    // ID_0 = Timer A input divider: 0 - /1 */
    TA2CTL = TASSEL1 + MC_2 + ID_0;

    TA2CCTL1 = OUTMOD_4; // Timer A2 Capture Compare Control Register auf Togglen setzen

    P2DIR |= BIT4; // Pin als Ausgang festlegen    
    P2SEL |= BIT4; // Peripherie Mode des Pins           

}
