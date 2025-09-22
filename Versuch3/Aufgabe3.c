#include <MSP430.h> //Register des Prozessors

void main(void){

	// WDTCTL = Watchdog Timer Control
    // WDTPW = Watchdog Timer Password
	WDTCTL = WDTPW + WDTHOLD;

	P1DIR = BIT0;    // P1.0 als Ausgang (LED1)
	P1REN = BIT1;    // P1.1 Pull Resistor
    P1OUT = BIT1;    // P1.1 Pull-Up aktivieren
	P1IES = BIT1;	 // Port 1 Interrupt Edge Select Register (Fallende Flanke da PullUp)
	P1IE = BIT1;	 // Port 1 Interrupt Enable Register

	P2REN = BIT1;    // P2.1 Pull Resistor
    P2OUT = BIT1;    // P2.1 Pull-Up aktivieren
	P2IES = BIT1;	 // Port 1 Interrupt Edge Select Register (Fallende Flanke da PullUp)
	P2IE = BIT1;	 // Port 2 Interrupt Enable Register

	// Globale Interruptfreigabe + Energiesparen
	__bis_SR_register(LPM4_bits + GIE);

}

// ISR für Taster 1
__attribute__((interrupt(PORT1_VECTOR)))
void P1_VECTOR_ISR(void){
	if (P1IFG & BIT1) {
		// LED einschalten
		P1OUT |= BIT0;
		P1IFG &= ~BIT1; //IR Bit clearen

		// Timer zurücksetzen und starten
	}
}

// ISR für Taster 2
__attribute__((interrupt(PORT2_VECTOR)))
void P2_VECTOR_ISR(void){
	if (P2IFG & BIT1) { 
		// LED ausschalten
		P1OUT |= BIT0;
		P2IFG &= ~BIT1; //IRBit clearen

		// Timer zurücksetzen und starten
	}
}

// ISR für Timer erstellen die nach Ablauf der eingestellten Zeit (4 oder 6 Sekunden) die LED ausschaltet
// ISR Bit evtl clearen; lokalen ISR enablen

//////////////////NOCH UNVOLLSTÄNDIG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!