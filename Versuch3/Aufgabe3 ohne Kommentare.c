#include <MSP430.h>

void main(void){

	WDTCTL = WDTPW + WDTHOLD;

	P1DIR |= BIT0; 

	P1REN |= BIT1;    
    P1OUT |= BIT1;    
	P1IES |= BIT1;	  
	P1IE |= BIT1;	  

	P2REN |= BIT1;    
    P2OUT |= BIT1;    
	P2IES |= BIT1;	  
	P2IE |= BIT1;	  

	__bis_SR_register(LPM4_bits + GIE);

}


__attribute__((interrupt(PORT1_VECTOR)))
void P1_VECTOR_ISR(void) {
    if (P1IFG & BIT1) {              
        P1OUT |= BIT0;               

        TA0CCR0 = 4096 * 4;          

        TA0CCTL0 = CCIE;         

        TA0CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;     
    }
    P1IFG = 0;
}


__attribute__((interrupt(PORT2_VECTOR)))
void P2_VECTOR_ISR(void) {
    if (P2IFG & BIT1) {  
        P1OUT |= BIT0;               

        TA0CCR0 = 4096 * 6;  

        TA0CCTL0 = CCIE;  
		
        TA0CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;     
    }
    P2IFG = 0; 
}


__attribute__((interrupt(TIMER0_A0_VECTOR)))
void TIMER0_A0_ISR(void) {
    P1OUT &= ~BIT0;      

    TA0CTL = 0;               
    TA0CCTL0 &= ~CCIFG;      
}
