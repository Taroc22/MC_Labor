#include <msp430.h>

void main(void){

    WDTCTL = WDTPW + WDTHOLD;

    // TA0CTL = Timer A0 Control Register
    // TASSEL1 = Timer A Source Select 1(ACLK)
    // MC_2 = Mode Control 2 (Continuous Mode)
    // ID_0 = Timer A input divider: 0 - /1 */
    TA0CTL = TASSEL1 + MC_2 + ID_0;

	P1DIR = BIT0;    // P1.0 als Ausgang (LED1)

	while(1){
        
        // Ist das Timer A Interrupt Flag gesetzt (Overflow)? (Bit 0 in TA0CTL)
        if (TA0CTL&TAIFG) {
            P1OUT ^= BIT0;   // LED1 toggeln
            TA0CTL &= ~TAIFG; // Reset des Timer A Interrupt Flags
        }
       
	}
}
