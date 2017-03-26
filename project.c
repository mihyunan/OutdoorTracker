#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <math.h>

#define FOSC 7372800
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
#define pi 3.14159265358979323846

volatile unsigned char temp_recv = 0;
volatile unsigned int count = 0;
volatile unsigned char gps_data[150];
unsigned char latitude[15];
unsigned char longitude[15];
float latitude_f, longitude_f;
unsigned char direction[3];
//strings to test lcd
const unsigned char str1[] = ">> at328-5.c hi <<901234";
const unsigned char str2[] = ">> USC EE459L <<78901234";

const unsigned char lat[] = "LAT:";
const unsigned char lon[] = "LONG:";
const unsigned char dir[] = "DIR:";
const unsigned char dist[] = "DIST:";


/*
LCD functions
void initialize(void);
void strout(int, unsigned char *);
void cmdout(unsigned char);
void datout(unsigned char);
void nibout(unsigned char); */

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

void find_direction(double peer_latitude, double peer_longitude)
{
	if (peer_latitude > latitude_f)
		if (peer_longitude > longitude_f)
		{
			direction[0]= 'N';
			direction[1]= 'W';
		}
		else 
		{
			direction[0]= 'N';
			direction[1]= 'E';
		}
	else
		if (peer_longitude > longitude_f)
		{
			direction[0]= 'S';
			direction[1]= 'W';
		}
		else
		{
			direction[0]= 'S';
			direction[1]= 'E';
		}

			direction[2]= '\0';
}



/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  Function prototypes                                           :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double);
double rad2deg(double);

double find_distance(double lat1, double lon1, double lat2, double lon2) {
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  dist = dist * 1.609344; // Dist in KM
 
  return (dist);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
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
    
    init_serial();
	sei();
	float peer_latitude = 42.9;
	float peer_longitude = 90.9;
    while (1) {               // Loop forever
        
        if (count == 50) {
        	gps_data[count] = '\0';
        	//strout(0, (unsigned char *) recv_buf);
        	
        	parse_gps();
        	find_direction(peer_latitude,peer_longitude);
        	double distance = find_distance(latitude_f, longitude_f, peer_latitude, peer_longitude);
        	char distance_c[10];
        	sprintf(distance_c, "%f", distance);
        	strout(0, (unsigned char *) lat); //prints out "LAT:"
        	strout(0x40, (unsigned char *) lon); //prints out "LONG:"
        	strout(0x04, (unsigned char *) latitude); //prints out the actual value of latitude
        	strout(0x45, (unsigned char *) longitude); //prints out the actual value of longitude
        	strout(0x14, (unsigned char *) dir); //prints out "DIR:"
        	strout(0x18, (unsigned char *) direction); //prints out the direction
        	strout(0x54, (unsigned char *) dist); //prints out "DIST:"
        	strout(0x59, (unsigned char *) distance_c); //prints out the distance in KM


        	_delay_ms(500);
        	cmdout(1);
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