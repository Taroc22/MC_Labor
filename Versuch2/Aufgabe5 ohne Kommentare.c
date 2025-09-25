#include <msp430.h>

#define TIMER_INTERVAL 0xFFF
#define CTL_STEP 30

void main(void){


    WDTCTL = WDTPW + WDTHOLD;

    P1REN |= BIT1;
    P1OUT |= BIT1;  

    P2REN |= BIT1; 
    P2OUT |= BIT1;  

    TA2CTL = TASSEL1 + MC_1 + ID_0;
    TA2CCR0 = TIMER_INTERVAL; 

    TA2CCR2 = TIMER_INTERVAL / 2; 
    TA2CCTL2 = OUTMOD_3; 

    P2DIR |= BIT5; 
    P2SEL |= BIT5;         
    
    while(1){
        if (!(P1IN & BIT1)) {  
            if (TA2CCR2 >= CTL_STEP) { 
                TA2CCR2 -= CTL_STEP;
            } else {
                TA2CCR2 = 0;     
            }
            for(int i=0; i<2000; i++); 
        }

        if (!(P2IN & BIT1)) {	
            if (TA2CCR2 <= (TIMER_INTERVAL - CTL_STEP)) {  
                TA2CCR2 += CTL_STEP;
            } else {
                TA2CCR2 = TIMER_INTERVAL;  
            }
            for(int i=0; i<2000; i++);
        }
    }
}
