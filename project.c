#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <math.h>
#include <stdlib.h>

#define FOSC 7372800
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
#define pi 3.14159265358979323846

volatile unsigned char temp_recv = 0;
volatile unsigned int count = 0;
volatile unsigned char gps_data[150];
unsigned char latitude[15];
unsigned char longitude[15];
char lat_c[50], long_c[50];
float latitude_f, longitude_f;
unsigned char direction[3];
//strings to test lcd
const unsigned char str1[] = ">> at328-5.c hi <<901234";
const unsigned char str2[] = ">> USC EE459L <<78901234";

const unsigned char lat[] = "LAT:";
const unsigned char lon[] = "LONG:";
const unsigned char dir[] = "DIR:";
const unsigned char dist_c[] = "DIST:";


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
	int temp1;
	float temp2;
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
		temp1 = atof(latitude) / 100;
		temp2 = (atof(latitude) - temp1*100) / 60;
		latitude_f = temp1 + temp2;
		//temp2 = temp1 - (int)(temp1/100);
		//latitude_f = (int)(temp1/100) + temp2/60;


		k = 0;
		
		while (gps_data[i] != ',') {
			longitude[k++] = gps_data[i++];
		}
		longitude[k] = '\0';

		//strncpy(temp2, longitude+3, k-3);
		//temp2_f = atof(temp2);
		//longitude_f = atof(longitude[0]) * 100 + atof(longitude[1]) * 10 + atof(longitude[2]) + temp2_f / 60;
		//longitude_f = atoi(longitude[0]) * 100 + atoi(longitude[1]) * 10 + atoi(longitude[2]);
		//longitude_f = atof(longitude) / 100;
		//temp2 = temp1 - (int)(temp1/100);
		//longitude_f = (int)(temp1/100) + temp2/60;
		temp1 = atof(longitude) / 100;
		temp2 = (atof(longitude) - temp1*100) / 60;
		longitude_f = temp1 + temp2;
	}
}


void find_direction(double peer_latitude, double peer_longitude)
{ // 40 90 34 118
	if (peer_latitude > latitude_f)
	{
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
	}
	else
	{
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
	}
	direction[2]= '\0';
}
/*
void find_direction()
{ // 40 90 34 118
	if (latitude_f < 40)
	{
		if (longitude_f > 90)
		{
			direction[0]= 'N';
			direction[1]= 'E';
		}
		else 
		{
			direction[0]= 'N';
			direction[1]= 'W';
		}
	}
	else
*/

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  Function prototypes                                           :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double);
double rad2deg(double);

float find_distance(double lat1, double lon1, double lat2, double lon2) {
	
	double theta, dist;
	theta = lon1 - lon2;
	dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
	dist = acos(dist);
	dist = rad2deg(dist);
	dist = dist * 60 * 1.1515;
	dist = dist * 1.609344 * 1000; // Dist in M
	
	/*d_ew = (lon2 - lon1) * cos(lat1);
	d_ns = (lat2 - lat1);

	double dist = sqrt(d_ew * d_ew + d_ns * d_ns);*/
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
	float peer_latitude = 34.02116397106819;
	float peer_longitude = 118.28957498073578;
    while (1) {               // Loop forever
        
        if (count == 50) {
        	gps_data[count] = '\0';
        	//strout(0, (unsigned char *) recv_buf);
        	
        	parse_gps();
        	find_direction(peer_latitude,peer_longitude);
        	float distance = find_distance(latitude_f, longitude_f, peer_latitude, peer_longitude);
        	//distance = 10;
        	unsigned char distance_c[10];
        	sprintf(distance_c, "%4.1f", distance);

        	//snprintf(distance_c, 10,  "%4.2f", distance);
        	strout(0, (unsigned char *) lat); //prints out "LAT:"
        	strout(0x40, (unsigned char *) lon); //prints out "LONG:"
        	strout(0x04, (unsigned char *) latitude); //prints out the actual value of latitude
        	strout(0x45, (unsigned char *) longitude); //prints out the actual value of longitude
        	
        	//if (peer_longitude > longitude_f)
        	
        	strout(0x14, (unsigned char *) dir); //prints out "DIR:"
        	strout(0x18, (unsigned char *) direction); //prints out the direction
        	strout(0x54, (unsigned char *) dist_c); //prints out "DIST:"
        	strout(0x59, (unsigned char *) distance_c); //prints out the distance in KM
        	
        	//strout(0x19, (unsigned char *) longitude[0]);
        	//strout(0x20, (unsigned char *) longitude[1]);

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