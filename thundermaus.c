/*
Micro SSTC C-code, Version 1.1?, Steve Ward, May 2012

Extending the start up pulse here.

Some input/output descriptions:
PA0		not used
PA1		not used
PA2		signal voltage for analog input
PA3		potentiometer 1 voltage input (controls pulse width)
PA4		programming pin
PA5		programming pin
PA6		programming pin
PA7		potentiometer 2 voltage input (controls frequency for timer)

PB0		LED output
PB1		audio/internal pulse timer select (HI = audio, LO = timer)
PB2		tesla oscillator enable (LO = ENABLE, HI = DISABLE)
PB3		RESET (programming pin)

when programming the chip, set internal oscillator for 8MHZ, and BOD to 4.7V.
*/



#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define INTEGRATOR_MAX				4000	
#define INTEGRATOR_MIN				200		
#define COUNTER_MAX					1024	
#define COUNTER_MIN					8	
#define START_PULSE					40

uint16_t ADC_Read (uint8_t ADCpin);

volatile uint8_t mode_select;
uint32_t burst_per, pulse_per;
uint16_t pulse_width;
volatile uint16_t integrator, temp_ADC, counter, counter_copy, waiting_counter;
uint16_t i;

int main (void)
{
	//initialize micro controller inputs and outputs
	DDRA = 0b00000000;		//all of these pins are inputs
	DDRB = 0b00000111;		//PB0 is LED output, PB2 is tesla coil disable
	
	PORTB |= 0b00000100;	//SET coil DISABLE
	PORTB |= 0b00000001;	//SET LED OUTPUT
	PORTB |= 0b00000010;
	
	sei();			//this globally enables interrupts

	while(1)
	{
	
		//this is the setup for running the internal timer instead of the audio input
		
		TCCR1A	=	0b00000000;			//nothing to set for this timer register
		TCCR1B	=	0b00000000;			//set WGM to CTC (overflow when reaching OCR1A), also 8MHZ counting
		TIMSK1	=	0b00000000;

		ADMUX 	=	0b00000000;			
		ADCSRA	=	0b00000000;			
		ADCSRB 	=	0b00000000;			
		
		TCCR0A	=	0b11000011;			//set for inverted pulse output and fast PWM, to be used as a one shot
		TCCR0B	=	0b00000011;			//set clock for /64, so 8uS per count.
		TIMSK0 	=	0b00000010;			//enable compare match A interrupt, this is used to terminate the one shot
		TCNT0	=	254;				//this preps the timer used as a one-shot
		OCR0A	=	0;
		
		ADC_Read(7);	//this initializes the ADC to catch all cases.
		
		while(mode_select == 0)	//pulse timer code needs to run for this
		{

			burst_per = ADC_Read(7);
			burst_per = burst_per<<6;
			pulse_per = ADC_Read(3);
			if(pulse_per > 1000)
			{
				mode_select = 1;
				break;
			}
			
			pulse_width = pulse_per;
			pulse_width = (pulse_width >> 4) + 3;
			
			pulse_per = (pulse_per<<5)+1000;
			
				if(burst_per < 513)
				{
					PORTB &= 0b11111101;	
					OCR0A = pulse_width;
					TCCR0B	=	0b00000011;			//set clock for /64, so 8uS per count.
					for (i = 0; i < START_PULSE; i++);
						_delay_us(1);
					PORTB |= 0b00000010;
					for (i = 0; i < pulse_per; i++);
						_delay_us(1);
				}
				else
				{
					for(uint8_t i = 0; i < 4; i++)
					{
						PORTB &= 0b11111101;
						OCR0A = pulse_width;
						TCCR0B	=	0b00000011;			//set clock for /64, so 8uS per count.
						for (i = 0; i < START_PULSE; i++);
							_delay_us(1);
						PORTB |= 0b00000010;
						for (i = 0; i < pulse_per; i++);
							_delay_us(1);
					}
					for (i = 0; i < burst_per; i++);
						_delay_us(1);
				}
				
		}
		
		//this is the set up for the audio input stuff.
		
		TCCR1A	=	0b00000011;			//
		TCCR1B	=	0b00011001;			//set WGM to CTC (overflow when reaching OCR1A), also 8MHZ counting
		TIMSK1	=	0b00000001;
		OCR1A	=	200;				//at 8mhz/200 we get 40ksps on the ADC trigger
		OCR1B 	=	100;
	
		ADMUX 	=	0b00000010;			//set ADMUX to PA2, which is audio input
		ADCSRA	=	0b11100011;			//set clock for /8 and start conversion and enable ADC auto trigger, enabled interrupt
		ADCSRB 	=	0b00000110;			//set up trigger event for TCC1 overflow
		
		waiting_counter = 0;			//this is used to decide if the control should revert to the timer
	
		while(mode_select == 1)
		{
			if(counter_copy)
			{	
				OCR0A = counter_copy >> 3;		//this sets the output pulse width
				counter_copy = 0;				//zero it back out for next pulse
				TCCR0B	=	0b00000011;			//set clock for /64, so 8uS per count.
				PORTB &= 0b11111101;
				for (i = 0; i < START_PULSE; i++);
					_delay_us(1);
				PORTB |= 0b00000010;
			}
			if(waiting_counter > 40000)			//no audio triggers have happened for 1 second
			{
				waiting_counter = 0;
				mode_select = 0;				//switch back to timer mode
				break;
			}
		}
	}	
	return 1;
}
ISR(TIM1_OVF_vect)				//new ADC data is ready to be used
{
	temp_ADC = ADC;
	
	if(temp_ADC > 537)
	{
		temp_ADC = temp_ADC - 537;
		integrator = (integrator + temp_ADC);
		if(integrator > INTEGRATOR_MAX)
		{
			integrator = INTEGRATOR_MAX;
		}
		counter++;
		if(counter > COUNTER_MAX)
		{
			counter = COUNTER_MAX;
		}
	}
	else
	{
		waiting_counter++;
		if(counter > COUNTER_MIN)
		{
			if (integrator > INTEGRATOR_MIN)
			{
				counter_copy = counter;
				waiting_counter = 0;
			}
		}
		integrator = 0;
		counter = 0;
	}
}

ISR(TIM0_COMPA_vect)
{				
	TCCR0B = 0b00000000;		//stop timer counter
	TCNT0 = 254;				//set it up so it starts just before zero
}

uint16_t ADC_Read (uint8_t ADCpin)
{
//This ADC reader returns a 10 bit result as a 16 bit unsigned integer.
//The MCU is tied up in this function until the result is obtained!

	ADMUX = ADCpin;          						//set ADMUX to look at ADCpin
	ADCSRA = 0b11000110;     						//start AD conversion with clock prescalar of /8
	while((ADCSRA&0b00010000) == 0)      			//get stuck here until flag is set
	{;}
	ADCSRA |= 0b00010000;
	return ADC;
}
