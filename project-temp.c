#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <math.h>

#define FOSC 7372800
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
#define pi 3.14159265358979323846
#define PRESCALAR 256
 

volatile unsigned char temp_recv = 0;
volatile unsigned int count = 0;
volatile unsigned char gps_data[150];
unsigned char latitude[15];
unsigned char longitude[15];
float latitude_f, longitude_f;
unsigned char direction[3];
//strings to test lcd
/*const unsigned char str1[] = ">> at328-5.c hi <<901234";
const unsigned char str2[] = ">> USC EE459L <<78901234";*/

const unsigned char lat[] = "LAT:";
const unsigned char lon[] = "LONG:";
const unsigned char dir[] = "DIR:";
const unsigned char dist[] = "DIST:";


const unsigned char findon[] = "ON ";
const unsigned char findoff[] = "OFF";
const unsigned char arrow[] = ">";
//const unsigned char space[] = " ";
volatile unsigned int buttonpressed = 0;
volatile unsigned int buttonstate = 0;
volatile unsigned int findstate = 0;
volatile unsigned int emergencystate = 0;
volatile unsigned int buzzerstate = 0;
volatile unsigned int buzz = 0;
volatile unsigned int change = 0;
//volatile unsigned int buzzerstate = 0;

void change_scroll() {
	strout(0x0a, (unsigned char *)" ");
	strout(0x4a, (unsigned char *)" ");
	strout(0x1e, (unsigned char *)" ");
	strout(0x5e, (unsigned char *)" ");
	if (buttonstate == 0) { // 0x0a 
		strout(0x0a, (unsigned char *) arrow);
	}
	else if (buttonstate == 1) { //0x4a
		strout(0x4a, (unsigned char *) arrow);
	}
	else if (buttonstate == 2) { //0x1e
		strout(0x1e, (unsigned char *) arrow);
	}
	else if (buttonstate == 3) { //0x5e
		strout(0x5e, (unsigned char *) arrow);
	}
	
	if (findstate == 0) {
		strout(0x10, (unsigned char *) findoff);
	} else {
		strout(0x10, (unsigned char *) findon);
	}
}

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

void init_buttons() 
{
	PORTC |= (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4);
	PCICR |= (1 << PCIE1); //enabling pin change interrupts on Port C
	PCMSK1  |= ( (1 << PCINT12) | (1 << PCINT11) | (1 << PCINT10) | (1 << PCINT9) );	//setting bits in mask register
}

void init_buzzer()
{
	DDRB |= (1 << DD2);
}

void init_counter()
{
	TCCR1B |= (1 << WGM12);
	TIMSK1 |= (1 << OCIE1A);
	//OCR1A = 0.25 * FOSC / PRESCALAR;
	TCCR1B |= (1 << CS12);
}

void changeOCR(double delay) {
	OCR1A = delay * FOSC / PRESCALAR;
}

void parse_gps()
{
	//unsigned char temp_buf[150];
	int i=0;
	int k=0;
	
	while (gps_data[i] != '$') {
		i++;
	}
	if ((gps_data[i+1] == 'G') && (gps_data[i+2] == 'P') && (gps_data[i+3] == 'G') 
			&& (gps_data[i+4] == 'G') && (gps_data[i+5] == 'A')) {
		i += 18;
		while (gps_data[i] != ',')
		{
			latitude[k++] = gps_data[i++];
		}
		i += 3;
		latitude[k] = '\0';
		latitude_f = atof(latitude);
		k = 0;
		
		while (gps_data[i] != ',') {
			longitude[k++] = gps_data[i++];
		}
		longitude[k] = '\0';
		longitude_f = atof(longitude);
	}
}


void init_serial() 
{
	DDRD |= (1 << PIND0);//PORTD pin0 as INPUT for RX
	UCSR0B |= (1 << RXCIE0);
	UBRR0 = MYUBRR;
	UCSR0B |= (1 << TXEN0 | 1 << RXEN0);
	UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

int main(void) {
    initialize();               // Initialize the LCD display
    cmdout(1);
    init_setting();
    
    init_serial();
    init_buttons();
    init_buzzer();
    init_counter();
    changeOCR(0.25);
    //change_scroll();
	sei();
	float peer_latitude = 42.9;
	float peer_longitude = 90.9;
    while (1) {               // Loop forever
        if (buttonpressed == 1) {
        	change_scroll();
        	_delay_ms(120);
        	buttonpressed = 0;
        }
        
        if (change == 10) {
        	cli();
        	changeOCR(0.1);
        	sei();
        	change =0;
        }
        
        if (count == 50) {
        	gps_data[count] = '\0';
        	//strout(0, (unsigned char *) recv_buf);
        	
        	parse_gps();
        	//find_direction(peer_latitude,peer_longitude);
        	//double distance = find_distance(latitude_f, longitude_f, peer_latitude, peer_longitude);
        	//char distance_c[10];
        	//sprintf(distance_c, "%f", distance);
        	/*strout(0, (unsigned char *) lat); //prints out "LAT:"
        	strout(0x40, (unsigned char *) lon); //prints out "LONG:"
        	strout(0x04, (unsigned char *) latitude); //prints out the actual value of latitude
        	strout(0x45, (unsigned char *) longitude); //prints out the actual value of longitude
        	strout(0x14, (unsigned char *) dir); //prints out "DIR:"
        	strout(0x18, (unsigned char *) direction); //prints out the direction
        	strout(0x54, (unsigned char *) dist); //prints out "DIST:"
        	strout(0x59, (unsigned char *) distance_c); //prints out the distance in KM*/


        	_delay_ms(500);
        	
        	//cmdout(1); 
        	count = 0;
        }
        
    	
    }

    return 0;   /* never reached */
}

ISR(USART_RX_vect)
{
	temp_recv = rx_char();
	if (count < 50) {
		gps_data[count] = temp_recv;
		count++;
	}
	
}

ISR(PCINT1_vect)
{
	if (buttonpressed == 0 && buttonstate == 0 && (PINC & 0x02) == 0x00) { //first line and down is pressed
		buttonstate = 1;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 1 && (PINC & 0x02) == 0x00) { //second line and down is pressed
		buttonstate = 2;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 1 && (PINC & 0x08) == 0x00) { //second line and up is pressed
		buttonstate = 0;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 2 && (PINC & 0x02) == 0x00) { //third line and down is pressed
		buttonstate = 3;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 2 && (PINC & 0x08) == 0x00) { //third line and up is pressed
		buttonstate = 1;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 3 && (PINC & 0x08) == 0x00) { //fourth line and up is pressed
		buttonstate = 2;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 0 && (PINC & 0x04) == 0x00 && findstate == 0) {
		findstate = 1;
		buzzerstate = 2;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 0 && (PINC & 0x04) == 0x00 && findstate == 1) {
		findstate = 0;
		buzzerstate = 0;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 1 && (PINC & 0x04) == 0x00) {
		strout(0x54, (unsigned char*) "Sent Msg1");
	}
	else if (buttonpressed == 0 && buttonstate == 2 && (PINC & 0x04) == 0x00) {
		strout(0x54, (unsigned char*) "Sent Msg2");
	}
	else if (buttonpressed == 0 && buttonstate == 3 && (PINC & 0x04) == 0x00) {
		strout(0x54, (unsigned char*) "Sent Msg3");
	}
	else if (buttonpressed == 0 && emergencystate == 0 && (PINC & 0x10) == 0x00) {
		strout(0x14, (unsigned char*) "Emergency!");
		buzzerstate = 1;
		emergencystate = 1;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && emergencystate == 1 && (PINC & 0x10) == 0x00) {
		strout(0x14, (unsigned char*) "off        ");
		buzzerstate = 0;
		emergencystate = 0;
		buttonpressed = 1;
	}
	
	//select = 0x04
	//emergency = 0x0d
}

ISR(TIMER1_COMPA_vect)
{
	if (buzzerstate == 2)
	{
		if (buzz == 0) {
			PORTB |= (1 << PB2);
			buzz = 1;
		} else {
			PORTB &= ~(1 << PB2);
			buzz = 0;
		}
		change++;
	} 
	else if (buzzerstate == 1) {
		PORTB |= (1 << PB2);
	}
	else if (buzzerstate == 0) {
		PORTB &= ~(1 << PB2);
	}
}