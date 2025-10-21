#include <msp430.h>
#include "ST7735.h"

const char* Texte[]= {"Einstellungen", "Telefonbuch", "Anrufe", "Klingelton", "Lautst\x84rke", "Seppi", "Karl Heinz","Carlo", "24.03.21", "Tutut","BeepBeep", "0 %","50 %","100 %","0162-123235","0163-143235","07243-123665","07212-213665","Zurueck",""};
int Select[][5]= { {0,1,2,19,19}, {3,4,18,19,19}, {5,6,18,19,19},{7,8,18,19,19},{9,10,18,19,19},{11,12,13,18,19},{14,18,19,19,19},{15,16,17,18,19}};

volatile signed Page =0;volatile signed PagePos=0;volatile signed PagePosAlt=1;volatile signed PageAlt; volatile signed PagePosLevel1; volatile signed PageAltLevel1;
int S1Alt; int S2Alt;
int Level=0;



void PrintMenue(){
	for(int j =0; j<=3 ; j++)
		
    	drawTextLine(j+1, 0, (char*) Texte[Select[Page][j]], 0x000000L,0xFFFFFFL);
}

void PrintSelection(){    
	drawTextLine(PagePosAlt+1, 0, (char*) Texte[Select[Page][PagePosAlt]], 0x000000L,0xFFFFFFL);
	drawTextLine(PagePos+1, 0, (char*) Texte[Select[Page][PagePos]], 0x000000L,0xFF00FFL);
	PagePosAlt=PagePos;
	PageAlt=Page;
}

void ClearMenue(){
	for (int i = 0; i < 11; i++)
		drawTextLine(i, 0, "", 0x000000L, 0xFFFFFFL);
}

void RefreshMenue(){
	
	drawTextLine(0, 0, "Auswahl", 0x000000L,0xFFFFFFL);
	PrintMenue();
	PrintSelection();
}
	
void UpDown (){
	int S2Neu = P3IN&BIT7;
	int S1Neu = P4IN&BIT0;

	if ((S1Neu!=S1Alt)&&S1Neu==0) {
		if (PagePos!=0) {
			PagePos--;
		}
		else {
			if((Select[Page][3])==18)
				PagePos=3;
			else if ((Select[Page][2])==18)
				PagePos=2;
			else if ((Select[Page][1])==18)
				PagePos=1;
			else if ((Select[Page][0])==18)
				PagePos=0;	
			else PagePos=2;
		}
		
	}

	if ((S2Neu!=S2Alt)&&S2Neu==0) {
		if (((Select[Page][PagePos+1])!=19)&&(PagePos!=3)){
			PagePos++;
		}
		else {PagePos=0;}
		
	}
	S1Alt=S1Neu;
	S2Alt=S2Neu;
}



void main(void) {
    WDTCTL = WDTPW | WDTHOLD;

	P1REN |= BIT1;
	P1OUT |= BIT1;
	P1IE |= BIT1;
	P1IES |= BIT1;

	P3REN|=BIT7;
	P3OUT|=BIT7;
	
	P4REN|=BIT0;
	P4OUT|=BIT0;

	S2Alt=P3IN&BIT7;
	S1Alt=P4IN&BIT0;

	ST7735_interface_init();
	ST7735_display_init();
	ClearMenue();
	PageAlt = Page;
	PagePosAlt = PagePos;
	RefreshMenue();

	TB0CTL=TBSSEL__ACLK + MC__UP;
	TB0CCTL0=CCIE;
	TB0CCR0=4000;

	while (1)
	{
		if(Page!=PageAlt) {
			RefreshMenue();
		}
		
		else if(PagePos!=PagePosAlt) {
			PrintSelection();
		}
		
		__bis_SR_register(GIE);
		
		
	}
	
	
}

__attribute__((interrupt(PORT1_VECTOR)))
void PORT1_VECTOR_ISR(void){

		
		if (Page==0){
			Page=PagePos+1;
			PagePos=0;
			Level=1;
		}
		
		else if(Level==1){
			
			if(PagePos==2) {
			PagePos=Page-1;
			Page=0;
			Level=0;
			}
			
			else if(Page==1){
				PageAlt=Page;
				PageAltLevel1=Page;
				Page=PagePos+4;
				PagePosLevel1=PagePos;
				PagePos=0;
				Level=2;
			}
			
			else if(Page==2){
				PageAlt=Page;
				PageAltLevel1=Page;
				Page=PagePos+6;
				PagePosLevel1=PagePos;
				PagePos=0;
				Level=2;
			}
			}
		
		
		else if(Level==2){
			
			if((Select[Page][PagePos])==18){
				Page=PageAltLevel1;
				PagePos=PagePosLevel1;
				Level=1;
				RefreshMenue();
			}
			
		}
		
		P1IFG&=~BIT1;
}

__attribute__((interrupt(TIMER0_B0_VECTOR)))
void TIMER0_B0_VECTOR_ISR(void){
	UpDown();
	
	

}