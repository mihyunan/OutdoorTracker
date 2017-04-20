#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOSC 7372800
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
#define PRESCALAR 256

volatile unsigned char temp_recv = 0;
volatile unsigned int count = 0;
volatile unsigned char gps_data[101];
char latitude[15];
char longitude[15];
float latitude_f, longitude_f;
volatile float start;
float distance;
volatile unsigned char direction[3];


// receives a character
char rx_char()
{
	while (!(UCSR0A & (1 << RXC0))) { }
	return UDR0;
}

// transmits a character
void tx_char(char ch)
{
	while ((UCSR0A & (1 << UDRE0)) == 0) { }
	UDR0 = ch;
}

void init_serial() 
{
	DDRD |= (1 << PIND0);//PORTD pin0 as INPUT for RX
	DDRD |= (1 << DD1); //PORTD Pin 1 as Output for TX
	UCSR0B |= (1 << RXCIE0);
	UBRR0 = MYUBRR;
	UCSR0B |= (1 << TXEN0 | 1 << RXEN0);
	UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}


int main(void) {
    init_serial();
    
	sei();

    while (1) {               // Loop forever
        tx_char(temp_recv);
    }

    return 0;   /* never reached */
}

ISR(USART_RX_vect)
{
	temp_recv = rx_char();
	/*if (count < 100) {
		gps_data[count] = temp_recv;
		count++;
	}*/
	
}