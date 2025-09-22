#include <MSP430.h> //Register des Prozessors


void main(void){

	// WDTCTL = Watchdog Timer Control
    // WDTPW = Watchdog Timer Password
    // WDTCNTCL = Watchdog Counter Clear
    // WDTSSEL_1 = Watchdog Source Select 1(ACLK)
    // WDTIS_4 = Watchdog Timer Intervall Select 4(ca.1s)
	// WDTTMSEL = Watchdog Timer Mode Select auf "Interval Timer Mode" setzen
	WDTCTL = WDTPW + WDTCNTCL + WDTSSEL_1 + WDTIS_4 + WDTTMSEL;
	SFRIE1 = WDTIE; // Setzen des "Watch Dog Interrupt Enable" im "Special Function Register Interrupt Enable"

	P1DIR = BIT0;    // P1.0 als Ausgang (LED1)

	// Globale Interruptfreigabe + Energiesparen
	__bis_SR_register(LPM4_bits + GIE);

}

// ISR für Watchdog Interrupt
__attribute__((interrupt(WDT_VECTOR)))
void WDT_VECTOR_ISR(void){
	P1OUT ^= BIT0;   // LED1 toggeln
}
