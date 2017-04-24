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
int messageState;

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

void init_dip_led()
{
	PORTC |= ((1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4));  //Enable Pull up resistors
	DDRB |=  ((1<<DD1) | (1<<DD2));


}
void send_message()
{
	if (messageState == 0)
		tx_char(0x30);
	else if (messageState == 1)
		tx_char(0x31);
	else if (messageState == 2)
		tx_char(0x32);
	else if (messageState == 3)
		tx_char(0x5b);
	else if (messageState == 4)
		tx_char(0x5c);
	else if (messageState == 5)
		tx_char(0x5d);
}


void color_led()
{
	if (temp_recv == 0x30) { //In danger
		PORTB |= (1<<PB1);
		PORTB &= ~(1<<PB2);
	}
	else if (temp_recv == 0x31 || temp_recv == 0x32) {
		PORTB |= (1<<PB2);
		PORTB &= ~(1<<PB1);
	}
	
}

int main(void) {
    init_serial();
    init_dip_led();
	sei();



    while (1) {               // Loop forever
        //tx_char(temp_recv);

        if (((PINC & (1<<PC1)) == 0) && ((PINC & (1<<PC2)) == 0)) //Message 00 In danger
		{
			messageState =0;
		}
		if (((PINC & (1<<PC1)) == 0) && ((PINC & (1<<PC2)) == 0x04)) //Message 01 I'm fine
		{
			messageState =1;
		}
		if (((PINC & (1<<PC1)) == 0x02) && ((PINC & (1<<PC2)) == 0)) //Message 10 Wait
		{
			messageState =2;
		}
		if (((PINC & (1<<PC3)) == 0) && ((PINC & (1<<PC4)) == 0)) //Message 00 In danger
		{
			messageState =3;
		}
		if (((PINC & (1<<PC3)) == 0) && ((PINC & (1<<PC4)) == 0x10)) //Message 01 I'm fine
		{
			messageState =4;
		}
		if (((PINC & (1<<PC3)) == 0x08) && ((PINC & (1<<PC4)) == 0)) //Message 10 Wait
		{
			messageState =5;
		}
        send_message();
        color_led();
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

ISR(PCINT1_vect) //Pin Change Interrupt Request 1 (Port C)
{
	if (((PINC & (1<<PC1)) == 0) && ((PINC & (1<<PC2)) == 0)) //Message 00 In danger
	{
		messageState =0;

	}
	else if (((PINC & (1<<PC1)) == 0) && ((PINC & (1<<PC2)) == 1)) //Message 01 I'm fine
	{
		messageState =1;


	}
	else if (((PINC & (1<<PC1)) == 1) && ((PINC & (1<<PC2)) == 0)) //Message 10 Wait
	{
		messageState =2;

	}

	//GPS if needed


	if (((PINC & (1<<PC3)) == 0) && ((PINC & (1<<PC4)) == 0)) //Message 00 In danger
	{
		messageState =3;

	}
	else if (((PINC & (1<<PC3)) == 0) && ((PINC & (1<<PC4)) == 1)) //Message 00 In danger
	{
		messageState =4;

	}


}