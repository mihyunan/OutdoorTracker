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
*
* Revision History
* Date     Author      Description
* 11/17/05 A. Weber    First Release for 8-bit interface
* 01/13/06 A. Weber    Modified for CodeWarrior 5.0
* 08/25/06 A. Weber    Modified for JL16 processor
* 05/08/07 A. Weber    Some editing changes for clarification
* 06/25/07 A. Weber    Updated name of direct page segment
* 08/17/07 A. Weber    Incorporated changes to demo board
* 01/15/08 A. Weber    More changes to the demo board
* 02/12/08 A. Weber    Changed 2nd LCD line address
* 04/22/08 A. Weber    Added "one" variable to make warning go away
* 04/15/11 A. Weber    Adapted for ATmega168
* 01/30/12 A. Weber    Moved the LCD strings into ROM
* 02/27/12 A. Weber    Corrected port bit assignments above
* 11/18/13 A. Weber    Renamed for ATmega328P
*************************************************************/

#include <avr/io.h>
#include <util/delay.h>


void initialize(void);
void strout(int, unsigned char *);
void cmdout(unsigned char);
void datout(unsigned char);
void nibout(unsigned char);

/*
  Use the "PROGMEM" attribute to store the strings in the ROM
  insteat of in RAM.
*/

const unsigned char str1[] = ">> at328-5.c hi <<901234";
const unsigned char str2[] = ">> USC EE459L <<78901234";

#define LCD_RS_B          0x80
#define LCD_E_D           0x20

#define LCD_Data_D     0xc0     // Bits in Port D for LCD data
#define LCD_Data_B     0x03     // Bits in Port B for LCD data
#define LCD_Status     0x80     // Bit in Port D for LCD busy status

#define WAIT           1
#define NOWAIT         0

int main(void) {
    unsigned char one = 1;

    DDRD |= LCD_Data_D;         // Set PORTD bits 6-7 for output
    DDRB |= LCD_Data_B;			// Set PORTB bits 0-1 for output
    
	DDRB |= LCD_RS_B;
	DDRD |= LCD_E_D;

    initialize();               // Initialize the LCD display

    strout(0, (unsigned char *) str1);    // Print string on line 1

    strout(0x40, (unsigned char *) str2); // Print string on line 2

    while (one) {               // Loop forever
    }

    return 0;   /* never reached */
}

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
    PORTD &= ~(LCD_E_D);   // Set R/W=0, E=0, RS=1
    PORTB |= LCD_RS_B;
    nibout(x);
    nibout(x << 4);
}

/*
  cmdout - Output a byte to the LCD display instruction register.  If
  "wait" is non-zero, wait for the busy flag to reset before returning.
  If "wait" is zero, return immediately since the BUSY flag isn't
  working during initialization.
*/
void cmdout(unsigned char x)
{
    PORTD &= ~LCD_E_D;         // Set R/W=0, E=0, RS=0
    PORTB &= ~LCD_RS_B;
    nibout(x);
    nibout(x << 4);
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
    PORTB |= (x & LCD_Data_B);  // Put low 2 bits of data in PORTB
    PORTB &= (x | ~LCD_Data_B);

    PORTD |= LCD_E_D;             // Set E to 1
    PORTD &= ~LCD_E_D;            // Set E to 0
}

/*
  initialize - Do various things to force a initialization of the LCD
  display by instructions, and then set up the display parameters and
  turn the display on.
*/
void initialize()
{
    _delay_ms(15);              // Delay at least 15ms

    nibout(0x30);       // Send a 0x30
    _delay_ms(4);               // Delay at least 4msec

    nibout(0x30);       // Send a 0x30
    _delay_us(120);             // Delay at least 100usec

    nibout(0x30);       // Send a 0x30

    nibout(0x20);         // Function Set: 4-bit interface
    
    cmdout(0x28);         // Function Set: 4-bit interface, 2 lines

    cmdout(0x0f);         // Display and cursor on
}
