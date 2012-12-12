// Thundermaus Test Code

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

int main (void)
{
	DDRA = 0b00000000;		//all of these pins are inputs
	DDRB = 0b00000111;		//PB0 is LED output, PB2 is tesla coil disable
	
	PORTB |= (1 << 2);	// set coil squeltch
	PORTB |= (1 << 0);	// set led
	PORTB |= (1 << 1);	// set gate squeltch
	
	while(1)
	{
		PORTB &= ~(1<<2);	// disable coil squeltch
		PORTB |= (1<<0);	// led on
		PORTB &= ~(1<<1);	// disable gate squeltch
		_delay_us(25);
		PORTB |= (1<<1);	// enable gate squeltch
		_delay_us(500);
		PORTB |= (1<<2);	// enable coil squeltch
		PORTB &= ~(1<<0);	// led off
		_delay_ms(950);
	}
}
