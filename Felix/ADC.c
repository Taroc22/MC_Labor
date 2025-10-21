#include <MSP430.h> //Register des Prozessors
#include "ST7735.h"
#include <stdio.h>


//JOY-Y: P6.3  A3
//JOY-X: P6.5  A5

int main(void) {

	unsigned int A3Value; 
	unsigned int A5Value;

	 WDTCTL = WDTPW | WDTHOLD;

	ADC12MCTL0 = ADC12INCH_3; // Memory 0 wird Kanal A3 zugeordnet
	ADC12MCTL1 = ADC12INCH_5; // Memory 1 wird Kanal A5 zugeordnet
	  
	
	// ADC12DIV_7: teilt ADC Taktquelle durch 7 -> lansamere Messung
	// ADC12CONSEQ_3: misst fortlaufend A3 und A5
    ADC12CTL1 = ADC12DIV_7 + ADC12CONSEQ_3;

	// ADC12ON: ADC einschalten
	// ADC12ENC: Enable Conversion -> Freigabe ADC Messung
	// ADC12SC: Start Conversion -> Messung starten
	ADC12CTL0 = ADC12ON + ADC12ENC + ADC12SC;       
	

	ST7735_interface_init(); // Initialisiert SPI-Schnittstelle zu Display
	ST7735_display_init(); // Initialisiert Display
    drawTextLine(0, 0, "X-Achse", 0x00FF00L,0x000000L);
	drawTextLine(2, 0, "Y-Achse", 0xFF00FFL,0x000000L);
	__bis_SR_register(GIE);
    while(1){
		A3Value=ADC12MEM0;
		A5Value=ADC12MEM1;
		char A3String[]="0000";
		char A5String[]="0000";
		
		A3String[3]+=(A3Value%10);
		A3String[2]+=(A3Value%100)/10;
		A3String[1]+=(A3Value%1000)/100;
		A3String[0]+=A3Value/1000;

		A5String[3]+=(A5Value%10);
		A5String[2]+=(A5Value%100)/10;
		A5String[1]+=(A5Value%1000)/100;
		A5String[0]+=A5Value/1000;


		drawTextLine(1, 0, A5String, 0xFFFFFFL,0x000000L);
		drawTextLine(3, 0, A3String, 0xFFFFFFL,0x000000L);

	}
}