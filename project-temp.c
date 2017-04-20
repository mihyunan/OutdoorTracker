#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOSC 7372800
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
#define PRESCALAR 256

float LEAVEY_LAT = 34.013183;
float LEAVEY_LONG = 118.169667;

volatile unsigned char temp_recv = 0;
volatile unsigned int count = 0;
volatile unsigned char gps_data[101];
char latitude[15];
char longitude[15];
float latitude_f, longitude_f;
volatile float start;
float distance;
volatile unsigned char direction[3];

volatile unsigned int buttonpressed = 0;
volatile unsigned int buttonstate = 0;
volatile unsigned int findstate = 0;
volatile unsigned int emergencystate = 0;
volatile unsigned int buzzerstate = 0;
volatile unsigned int buzz = 0;
volatile unsigned int change = 0;
volatile unsigned int rf_gps = 1;
volatile unsigned int messagestate = 1;

void change_scroll() {
	strout(0x0a, (unsigned char *)" ");
	strout(0x4a, (unsigned char *)" ");
	strout(0x1e, (unsigned char *)" ");
	strout(0x5e, (unsigned char *)" ");
	if (buttonstate == 0) { // 0x0a 
		strout(0x0a, (unsigned char *) ">");
	}
	else if (buttonstate == 1) { //0x4a
		strout(0x4a, (unsigned char *) ">");
	}
	else if (buttonstate == 2) { //0x1e
		strout(0x1e, (unsigned char *) ">");
	}
	else if (buttonstate == 3) { //0x5e
		strout(0x5e, (unsigned char *) ">");
	}
	
	if (findstate == 0) {
		strout(0x0B, (unsigned char *) "FIND OFF");
	} else {
		strout(0x0B, (unsigned char *) "FIND ON ");
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
	TCCR1B |= (1 << WGM12);
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
		
		k = 0;
		
		while (gps_data[i] != ',') {
			longitude[k++] = gps_data[i++];
		}
		longitude[k] = '\0';
		
		latitude_f = atof(latitude) / 100;
		longitude_f = atof(longitude) / 100;
	}
	
}

float convert_to_distance() 
{
	//float distance = sqrtf(pow(LEAVEY_LAT-latitude_f,2)+pow(LEAVEY_LONG-longitude_f,2))/0.0107548*1000;
	//return distance;
	
	
	//float x = (int)(LEAVEY_LAT * 1000000);
	float x = (LEAVEY_LAT*1000000 - ((int)LEAVEY_LAT*1000000)) / 10000;
	x = x - ((latitude_f*1000000 - ((int)latitude_f*1000000)) / 10000);
	x = fabs(x) / 0.856;
	
	float y = (LEAVEY_LONG*1000000 - ((int)LEAVEY_LONG*1000000)) / 10000;
	y = y - ((longitude_f*1000000 - ((int)longitude_f*1000000)) / 10000);
	y = fabs(y) / 1.044;
	//float x = ((int)(LEAVEY_LAT * 1000000) % 1000000 / 10000) - ((int)(latitude_f *1000000) % 1000000 / 10000);
	//float y = ((int)(LEAVEY_LONG * 1000000) % 1000000 / 10000) - ((int)(longitude_f *1000000) % 1000000 / 10000);
	//x = fabs(x) / 0.856;
	//y = fabs(y) / 1.044;
	//x = x/0.856;
	//y = y/1.044;
	float dst = sqrtf(pow(x,2)+pow(y,2));
	dst = 1609.34*dst; //convert miles to meters
	//float d = sqrtf(pow(LEAVEY_LAT-latitude_f,2)+pow(LEAVEY_LONG-longitude_f,2));
	if (dst > 10000) return latitude_f;
	else return dst;
	//return x;
}

float change_OCR_rate()
{
	int percent = (int)(distance/start * 100);
	percent = percent - (percent % 10);
	if (percent == 0) {
		return 0;
	}
	float percent_f = percent/100.0;
	changeOCR(0.7*percent_f);
	return percent_f;
}


void find_direction()
{ // 40 90 34 118
	if (LEAVEY_LAT > latitude_f)
	{
		if (LEAVEY_LONG > longitude_f)
		{
			direction[0]= 'N';
			direction[1]= 'W';
		}
		else 
		{
			direction[0]= 'N';
			direction[1]= 'E';
		}
	}
	else
	{
		if (LEAVEY_LONG > longitude_f)
		{
			direction[0]= 'S';
			direction[1]= 'W';
		}
		else
		{
			direction[0]= 'S';
			direction[1]= 'E';
		}
	}
	direction[2]= '\0';
}

void init_serial() 
{
	DDRD |= (1 << PIND0);//PORTD pin0 as INPUT for RX
	UCSR0B |= (1 << RXCIE0);
	UBRR0 = MYUBRR;
	UCSR0B |= (1 << TXEN0 | 1 << RXEN0);
	UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

void init_mux_select() 
{
	DDRD |= (1 << DD2);
}

void select_GPS()
{
	PORTD &= ~(1 << PD2);
}

void select_RF()
{
	PORTD |= (1 << PD2);
}

void send_message()
{
	if (messagestate == 0) {
		strout(0x54, (unsigned char*) "send msg0");
		tx_char(0x30);
	} else if (messagestate == 1) {
		strout(0x54, (unsigned char*) "send msg1");
		tx_char(0x31);
	} else if (messagestate == 2) {
		strout(0x54, (unsigned char*) "send msg2");
		tx_char(0x32);
	}
}


int main(void) {
    initialize();               // Initialize the LCD display
    cmdout(1);
    init_setting();
    
    init_serial();
    init_buttons();
    init_buzzer();
    init_mux_select();
    init_counter();
    changeOCR(1);
    
    select_GPS();
    //select_RF();
    
	sei();

    while (1) {               // Loop forever
        if (buttonpressed == 1) {
        	change_scroll();
        	send_message();
        	_delay_ms(200);
        	buttonpressed = 0;
        } 
        
        /*if (change == 10) {
        	changeOCR(0.1);
        	change = 0;
        } */
        
        send_message();
        
        if (rf_gps == 0) {
        	select_RF();
        } else {
        	select_GPS();
        }
        
        if (rf_gps == 0) {
        	if (temp_recv == 0x30) {
        		strout(0x14, (unsigned char*) "In Danger");
        	}
        	else if (temp_recv == 0x31) {
        		strout(0x14, (unsigned char*) "I'm Fine ");
        	}
        	else if (temp_recv == 0x32) {
        		strout(0x14, (unsigned char*) "Wait!    ");
        	}
        }
        
        
        if (count == 100) {
        	
        	
        	if (rf_gps == 1) {
        		gps_data[count] = '\0';
        		parse_gps();
        		find_direction();
        		distance = convert_to_distance();
        		char dist_c[15];
        		snprintf(dist_c, 15, "Dt:%4.1fm", distance);
        		strout(0x00, (unsigned char *) dist_c);
        		strout(0x44, (unsigned char *) direction);
        	}
        	
        	/*parse_gps();
        	find_direction();
        	distance = convert_to_distance();*/
        	float percent;
        	if (findstate == 1) {
        		percent = change_OCR_rate();
        	}
        	
        	//char dist_c[15];
        	//char percent_c[15];
        	//char x[15];
        	//char y[15];
        	//snprintf(dist_c, 15, "Dt:%4.1fm", distance);
        	//snprintf(percent_c, 15, "%4.2f", percent); 
        	//strout(0x00, (unsigned char *) gps_data);
        	//snprintf(x, 15, "%4.6f", latitude_f);
        	//snprintf(y, 15, "%4.6f", longitude_f);
        	//strout(0x00, (unsigned char *) dist_c);
        	//strout(0x44, (unsigned char *) direction);
        	//strout(0x44, (unsigned char *) percent_c);
        	//strout(0x14, (unsigned char *) x);
        	//strout(0x54, (unsigned char *) y);
        	//strout(0x14, (unsigned char *) latitude);
        	//strout(0x54, (unsigned char *) longitude);
        	
        	//_delay_ms(100);
        	count = 0;
        }
        
        /*if (count == 50) {
        	gps_data[count] = '\0';
        	
        	parse_gps();
        	//find_direction();
        	//float distance = convert_to_distance();
        	//char distance_c[10];
        	//sprintf(distance_c, "%4.4fkm", distance);
        	//strout(0x00, (unsigned char *) distance_c);
        	//sprintf(distance_c, "%f", latitude_f);
        	
        	//strout(0x14, (unsigned char *) latitude);
        	//strout(0x54, (unsigned char *) longitude);
        	//strout(0x44, (unsigned char *) direction);

        	_delay_ms(100);
        	
        	//cmdout(1); 
        	count = 0;
        } */
    	
    }

    return 0;   /* never reached */
}

ISR(USART_RX_vect)
{
	temp_recv = rx_char();
	if (rf_gps == 1 && count < 100) {
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
		start = distance;
		if (distance > 0) {
			findstate = 1;
			buzzerstate = 2;
		}
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 0 && (PINC & 0x04) == 0x00 && findstate == 1) {
		changeOCR(1);
		findstate = 0;
		buzzerstate = 0;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 1 && (PINC & 0x04) == 0x00) {
		//strout(0x54, (unsigned char*) "Sent Msg1");
		messagestate = 0;
	}
	else if (buttonpressed == 0 && buttonstate == 2 && (PINC & 0x04) == 0x00) {
		//strout(0x54, (unsigned char*) "Sent Msg2");
		messagestate = 1;
	}
	else if (buttonpressed == 0 && buttonstate == 3 && (PINC & 0x04) == 0x00) {
		//strout(0x54, (unsigned char*) "Sent Msg3");
		messagestate = 2;
	}
	else if (buttonpressed == 0 && emergencystate == 0 && (PINC & 0x10) == 0x00) {
		//strout(0x14, (unsigned char*) "Emergency!");
		buzzerstate = 1;
		emergencystate = 1;
		buttonpressed = 1;
	}
	else if (buttonpressed == 0 && emergencystate == 1 && (PINC & 0x10) == 0x00) {
		//strout(0x14, (unsigned char*) "off        ");
		buzzerstate = 0;
		emergencystate = 0;
		buttonpressed = 1;
	}
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
		//change++;
	} 
	else if (buzzerstate == 1) {
		PORTB |= (1 << PB2);
	}
	else if (buzzerstate == 0) {
		PORTB &= ~(1 << PB2);
	}
	change++;
	if (change == 8 && rf_gps == 0) 
	{
		rf_gps = 1;
		change = 0;
	} 
	else if (change == 2 && rf_gps == 1)
	{
		rf_gps = 0;
		change = 0;
	}
}