/*
 * ADASEphone.cpp
 *
 * Created: 25.02.2017 13:15:29
 * Author : sav6622
 * Repository : https://github.com/sav6622/ADASEphone
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

unsigned char out2trunk = 0;
unsigned char out2abonent = 0;

// Base frequency 7812,5 Hz (PWM for clock 16ÌÃö)
// True frequency ADASE F1 - 1201,9 Hz
// True frequency ADASE F2 - 1602,6 Hz
// True frequency 425 Hz - 400,6 Hz
#define LEN_SAMPLE 39		//Good number of samples for very accurate tone frequency F1 and F2
const unsigned char sinus_1200hz[LEN_SAMPLE] ={128,180,187,143,85,64,98,157,191,170,112,68,75,128,180,187,143,85,64,98,157,191,170,112,68,75,128,180,187,143,85,64,98,157,191,170,112,68,75};
const unsigned char sinus_1600hz[LEN_SAMPLE] ={128,189,162,85,70,138,191,152,78,75,148,191,143,72,82,157,190,133,68,89,166,187,122,65,98,173,183,112,64,107,180,177,103,64,117,185,170,93,66};
const unsigned char sinus_1200_1600hz[LEN_SAMPLE] ={128,241,221,100,28,74,161,182,141,117,132,132,90,72,134,217,205,90,5,60,195,250,165,50,38,121,183,165,123,123,138,114,73,94,181,227,155,34,14};
const unsigned char sinus_425hz[LEN_SAMPLE] ={128,156,181,201,213,217,211,197,175,149,120,93,68,50,40,39,47,63,86,113,142,169,192,208,216,215,205,187,162,135,106,80,58,44,38,42,54,74,99};
	 
volatile bool flag = false;			//Flag of interrupt
unsigned char state = 0;			//State of automation model

#define COUNT_LOGIC 78				//Discounting repeating of logic work to 100 Hz

 void setup() {
	 // setup ADC
	 ADMUX = 0x60; // left adjust, adc0, internal vcc
	 ADCSRA = 0xe5; // turn on adc, ck/32, auto trigger
	 ADCSRB =0x07; // t1 capture for trigger
	 DIDR0 = 0x01; // turn off digital inputs for adc0
	 
	//Set input for buttons
	PORTB = (1<<PB5)|(1<<PB4)|(1<<PB3)|(1<<PB2);
	DDRB = (0<<PB5)|(0<<PB4)|(0<<PB3)|(0<<PB2);
	
	//Set out for PWM PD6
	//Set out for mesure perfomance PD1
	PORTD = (1<<PD6)|(1<<PD1);
	DDRD = (1<<DDD6)|(1<<DDD1);

	//Settings PWM
	TCCR0A = 0b10000011;
	TCCR0B = 0b00001010;
	TCNT0 = 0;
	TIMSK0 = 0b00000001;
	TIFR0 = 0b00000000;
		 
	 sei();				// interrupt on
 }

int main(void)
{
	setup();
	
	unsigned char sample = 0;		//Number of current out sample
	unsigned char cntr_logic = 100;	//
	
    while (1) 
    {
		if (flag)	//Wait flag interrupt (frequency 7812,5 Hz)
		{
			flag = false;	//Reset flag
			
			PORTD = (1<<PD1);
			
			//Increment ptr to current sample
			sample++;
			if (sample >= LEN_SAMPLE)
				sample = 0;
			
			//Check buttons	
			char input = PINB;
			bool button1 = (input>>PB2)&0x1;
			bool button2 = (input>>PB3)&0x1;
			bool button3 = (input>>PB4)&0x1;
			bool button4 = (input>>PB5)&0x1;
			
			cntr_logic--;
			if (cntr_logic == 0){
				cntr_logic = COUNT_LOGIC;
				
				// Logic
				switch(state){
					case 0:		//Base state
						if (button1||button2||button3||button4)
							sample = 0;			//Reset counter for start from first sample
						if (button1)
							state = 1;
						if (button2)
							state = 2;
						if (button3)
							state = 3;
						if (button4)
							state = 4;
						break;
					case 1:		//Out F1
						if (!button1)
							state = 0;
						break;
					case 2:		//Out F2
						if (!button2)
							state = 0;
						break;
					case 3:		//Out F1 + F2
						if (!button3)
							state = 0;
						break;
					case 4:		//Out 425 Hz
						if (!button4)
							state = 0;
						break;
					default:
						state = 0;
						break;
				}
			}
			
			// Do
			switch(state){
				case 0:		//Base state
					out2abonent = 128;	//Set middle
					break;
				case 1:		//Out F1
					out2abonent = sinus_1200hz[sample];
					break;
				case 2:		//Out F2
					out2abonent = sinus_1600hz[sample];
					break;
				case 3:		//Out F1 + F2
					out2abonent = sinus_1200_1600hz[sample];
					break;
				case 4:		//Out 425 Hz
					out2abonent = sinus_425hz[sample];
					break;
				default:
					out2abonent = 128;	//Set middle
					break;
			}
			
			PORTD = (0<<PD1);
		}
		else
			sleep_cpu();
    }
}

ISR(TIMER0_OVF_vect) {
	//Set flag interrupt
	flag = true;
	
	// read ADC
	//unsigned int temp1 = ADCL; // low byte first
	//unsigned int temp2 = ADCH;
	
	OCR0A = out2abonent;
}

