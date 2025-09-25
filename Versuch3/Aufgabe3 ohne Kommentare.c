#include <MSP430.h> // Register des Prozessors

void main(void){

	// WDTCTL = Watchdog Timer Control
    // WDTPW = Watchdog Timer Password
	// WDTHOLD = Watchdog pausieren
	WDTCTL = WDTPW + WDTHOLD;

	P1DIR |= BIT0;    // P1.0 als Ausgang (LED1)

	P1REN |= BIT1;    // P1.1 Pull Resistor
    P1OUT |= BIT1;    // P1.1 Pull-Up aktivieren
	P1IES |= BIT1;	  // Port 1 Interrupt Edge Select Register (Fallende Flanke da PullUp)
	P1IE |= BIT1;	  // Port 1 Interrupt Enable Register

	P2REN |= BIT1;    // P2.1 Pull Resistor
    P2OUT |= BIT1;    // P2.1 Pull-Up aktivieren
	P2IES |= BIT1;	  // Port 1 Interrupt Edge Select Register (Fallende Flanke da PullUp)
	P2IE |= BIT1;	  // Port 2 Interrupt Enable Register

	// Globale Interruptfreigabe + Energiesparen
	__bis_SR_register(LPM4_bits + GIE);

}


// ISR für Taster 1 (P1.1)
__attribute__((interrupt(PORT1_VECTOR)))
void P1_VECTOR_ISR(void) {
    if (P1IFG & BIT1) {              // Prüfen ob das Interrupt-Flag für P1.1 gesetzt ist
        P1OUT |= BIT0;               // LED (P1.0) einschalten

        // Timer-Konfiguration für 4 Sekunden:
        TA0CCR0 = 4096 * 4;          // Capture/Compare Register 0
                                     // 4096 Takte pro Sekunde (ACLK = 32768 Hz / 8 Prescaler)
                                     // => 4 * 4096 = 16384 Takte = 4 Sekunden

        TA0CCTL0 = CCIE;             // Capture/Compare Control Register 0
                                     // CCIE: Capture/Compare Interrupt Enable (Interrupt auf CCR0 aktivieren)

		// TA0CTL: Timer_A Control Register
		// TASSEL_1: Timer_A Soruce Select ACLK (32768 Hz)
		// MC_1: Mode Control Up-Mode (0 → CCR0)
		// ID_3: Input Divider /8 Teiler
		// TACLR: Timer_A Counter Clear
        TA0CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;     
    }
    P1IFG = 0;                       // Interrupt-Flag-Register von Port1 löschen
}


// ISR für Taster 2 (P2.1)
__attribute__((interrupt(PORT2_VECTOR)))
void P2_VECTOR_ISR(void) {
    if (P2IFG & BIT1) {              // Prüfen ob das Interrupt-Flag für P2.1 gesetzt ist
        P1OUT |= BIT0;               // LED (P1.0) einschalten

        // Timer-Konfiguration für 6 Sekunden:
        TA0CCR0 = 4096 * 6;          // Capture/Compare Register 0
                                     // 4096 Takte pro Sekunde (ACLK = 32768 Hz / 8 Prescaler)
                                     // => 6 * 4096 = 24576 Takte = 6 Sekunden

        TA0CCTL0 = CCIE;             // Capture/Compare Control Register 0
                                     // CCIE: Capture/Compare Interrupt Enable (Interrupt auf CCR0 aktivieren)

		// TA0CTL: Timer_A Control Register
		// TASSEL_1: Timer_A Soruce Select ACLK (32768 Hz)
		// MC_1: Mode Control Up-Mode (0 → CCR0)
		// ID_3: Input Divider /8 Teiler
		// TACLR: Timer_A Counter Clear
        TA0CTL = TASSEL_1 + MC_1 + ID_3 + TACLR;     
    }
    P2IFG = 0;                       // Interrupt-Flag-Register von Port2 löschen
}

// Timer0_A0 ISR => wird bei CCR0 erreicht ausgelöst
__attribute__((interrupt(TIMER0_A0_VECTOR)))
void TIMER0_A0_ISR(void) {
    P1OUT &= ~BIT0;                  // LED (P1.0) ausschalten

    TA0CTL = 0;                      // Timer stoppen
    TA0CCTL0 &= ~CCIFG;              // Capture/Compare Interrupt Flag löschen
                                     // verhindert erneutes Aufrufen der ISR
}
