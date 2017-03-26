/*************************************************************
*       at328-5.c - Demonstrate interface to a parallel LCD display
*
*       This program will print a message on an LCD display
*       using the 4-bit wide interface method
*
*       PORTB, bit 4 (0x10) - output to RS (Register Select) input of display
*              bit 3 (0x08) - output to R/W (Read/Write) input of display
*              bit 2 (0x04) - output to E (Enable) input of display
*       PORTB, bits 0-1, PORTD, bits 2-7 - Outputs to DB0-DB7 inputs of display.
*
*       The second line of the display starts at address 0x40. test
*************************************************************/

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"

#define LCD_RS_B          0x80
#define LCD_E_B           0x01

#define LCD_Data_D     0xf0     // Bits in Port D for LCD data

const unsigned char dist[] = "DIST:";
const unsigned char find[] = "FIND ";
const unsigned char msg1[] = "In Danger";
const unsigned char msg2[] = "I'm Fine";
const unsigned char msg3[] = "Wait!";

/*
  strout - Print the contents of the character string "s" starting at LCD
  RAM location "x".  The string must be terminated by a zero byte.
*/
void strout(int x, unsigned char *s)
{
    unsigned char ch;

    cmdout(x | 0x80);     // Make A contain a Set Display Address command

    while ((ch = *s++) != (unsigned char) '\0') {
        datout(ch);     // Output the next character
    }
}

/*
  datout - Output a byte to the LCD display data register (the display)
  and wait for the busy flag to reset.
*/
void datout(unsigned char x)
{
    PORTB &= ~(LCD_E_B);   //E=0, RS=1
    PORTB |= LCD_RS_B;
    nibout(x);
    nibout(x << 4);
    _delay_ms(2);
}

/*
  cmdout - Output a byte to the LCD display instruction register.
*/
void cmdout(unsigned char x)
{
    PORTB &= ~LCD_E_B;         // Set R/W=0, E=0, RS=0
    PORTB &= ~LCD_RS_B;
    nibout(x);
    nibout(x << 4);
    _delay_ms(2);
}

/*
  nibout - Puts bits 4-7 from x into the four bits that we're
  using to talk to the LCD.  The other bits of the port are unchanged.
  Toggle the E control line low-high-low.
*/
void nibout(unsigned char x)
{
    PORTD |= (x & LCD_Data_D);  // Put high 4 bits of data in PORTD
    PORTD &= (x | ~LCD_Data_D);

    PORTB |= LCD_E_B;             // Set E to 1
    PORTB &= ~LCD_E_B;            // Set E to 0
}

/*
  initialize - Do various things to force a initialization of the LCD
  display by instructions, and then set up the display parameters and
  turn the display on.
*/
void initialize()
{
	DDRD |= LCD_Data_D;         // Set PORTD bits 6-7 for output
    
	DDRB |= LCD_RS_B;
	DDRB |= LCD_E_B;
	
    _delay_ms(15);              // Delay at least 15ms

    nibout(0x30);       // Send a 0x30
    _delay_ms(4);               // Delay at least 4msec

    nibout(0x30);       // Send a 0x30
    _delay_us(120);             // Delay at least 100usec

    nibout(0x30);       // Send a 0x30

    nibout(0x20);         // Function Set: 4-bit interface
    _delay_ms(2);
    
    cmdout(0x28);         // Function Set: 4-bit interface, 2 lines

    cmdout(0x0f);         // Display and cursor on
}

void init_setting(void)
{
	strout(0, (unsigned char *) dist);
	strout(0x0A, (unsigned char*) find);
	strout(0x4B, (unsigned char*) msg1);
	strout(0x1F, (unsigned char*) msg2);
	strout(0x5F, (unsigned char*) msg3);
}