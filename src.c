//CHANGE THE VALUE OF "SLEEP" TO THE NUMBER OF SECONDS YOU WISH TO SLEEP
//CONNECT BOARD USING USB TO MINI USB CABLE
//SWITCH 1: "USB"
//FLASH FROM CMD PROMPT FROM FOLDER CONTAINING THIS FILE USING THE FOLLOWING:

//avr-gcc -mmcu=atmega644p -DF_CPU=12000000 -Wall -Os src.c -o src.elf
//avr-objcopy -O ihex src.elf src.hex
//avrdude -c usbasp -p m644p -U flash:w:src.hex

//FLICK SWITCH 1 TO "EXT" WHEN NOT IN USE
//FLICK SWITCH 1 TO "USB" BEFORE GOING TO BED. THE COUNTDOWN WILL START

//COMPONENTS:
/*
r	 : Red LED PWM signal from board
g	 : Green ""
b	 : Blue ""
gnd  : Ground of board + ground wires
GND  : Ground connection from power cable
12V  : 12 Volt connection from power cable
R	 : Red connection from LED strip
G	 : Green ""
B	 : Blue ""
R1:R3: 110 Ohm resistor from Base to uC
R4-R9: 33 Ohm resistor from Emitter to GND
	L> can use any resistor if 12/R < max. rating of LED strip
PIN  : 4-pin needle connector where LED strip is plugged in
	L> check the LED strip for orientation!!!
NPNe : Emitter of NPN transistor
	L> check the datasheet for transistor pinouts
	L> I used a BD677
emit : Emitter connection wire
NPNc : Collector ""
NPNb : Base ""
base : Base connection wire
*/

//CIRCUIT DIAGRAM FOR ON-BOARD BREADBOARD:
/*   A    B    C    D    E    ||  F    G    H    I    J
  +--------------------------------------------------------+
1 |     	   gnd  R4        ||  R4   R5                  |
2 |                 R5   emit ||  emit NPNe                |
3 |  b                   R1   ||       NPNc           B    |
4 |       R1             base ||  base NPNb                |
5 |       gnd  gnd  R6        ||  R6   R7                  |
6 |                 R7   emit ||  emit NPNe                |
7 |  r                   R2   ||       NPNc           R    |
8 |       R2             base ||  base NPNb                |
9 |       gnd  gnd  R8        ||  R8   R9                  |
10|                 R9   emit ||  emit NPNe                |
11|  g                   R3   ||       NPNc           G    |
12|       R3             base ||  base NPNb                |
13|							  ||                           |
14|							  ||	      		 B    PIN  |
15|  gnd  gnd            GND  ||            R         PIN  |
16|                           ||       G			  PIN  |
17|                           ||  12V                 PIN  |
  +--------------------------------------------------------+
 */

//PORT WIRINGS:
/*
PORT B:
Pin |0 1 2 3 4 5 6 7 V Gnd|
----|---------------------|
Wire|      g              |

PORT D:
Pin |0 1 2 3 4 5 6 7 V Gnd|
----|---------------------|
Wire|            b r   gnd|
 */

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

//CHANGE THIS TO THE NUMBER OF SECONDS YOU WISH TO SLEEP
uint16_t SLEEPSECONDS = /*28800*/ 0;

//"volatile" keyword lets the program know the value is likely to change
volatile uint8_t red = 0;
volatile uint8_t green = 0;
volatile uint8_t blue = 0;

volatile uint8_t seconds = 0;
volatile uint8_t wait = 0;

void initialise(void);
void brightness(uint8_t redBrightness, uint8_t greenBrightness, uint8_t blueBrightness);
void colours(void);

//Interrupt Service Routine
ISR(TIMER1_COMPA_vect) {
	//Prevent the OCR values from wrapping back to 0
	if (red > 254){
		red = 254;
	}
	if (green > 254){
		green = 254;
	}
	if (blue > 254){
		blue = 254;
	}
	seconds ++;
	//Wait 8 hours(=28800 seconds) while the user sleeps
	if (seconds > SLEEPSECONDS){
		//Slow the colour changes by a factor of 4
		if (wait > 3){
			colours();
			wait = 0;
		} else {
			wait ++;
		}
	}
}

int main(void) {
	initialise();
	//Timer 1 Interrupt Mask Register, enable OCR1A Interrupt
	TIMSK1 |= _BV(OCIE1A);
	//Set Global Interrupt Enable
	sei();
	while (1) {
	}
	return 0;
}

#define WGM  0b0100//CTC mode, OCR1A MAX
#define COM1A 0b00//OC1A disconnected
#define CS1 0b100//Divide by 256

void initialise(){
	//Set Port D as output for Timer 1 (clock) and Timer 2 (R + B)
	DDRD = 0xFF;
	//Set Port B as output for Timer 0 (G)
	DDRB = 0xFF;

	//8-bit Timer 2 for R and B
	TCCR2A = 0b10100011;
	TCCR2B = 0b00000001;
	//8-bit Timer 0 for G
	TCCR0A = 0b10110011;
	TCCR0B = 0b00000001;

	//16-bit Timer 1 for clock
	//Set WGM to put Timer 1 into CTC mode with OCR1A as the top value
	//WGM1 is split across 2 registers, bits 3 and 2 are in TCCR1B and 1 and 0 in TCCR1A
	TCCR1A |= ((WGM & 0x01) << WGM10);
	TCCR1A |= ((WGM & 0x02) << (WGM11 - 1));
	TCCR1B |= ((WGM & 0x04) << (WGM12 - 2));
	TCCR1B |= ((WGM & 0x08) << (WGM13 - 3));
	//Set COM1A so OC1A will toggle on each compare match
	TCCR1A |= ((COM1A & 0x01) << COM1A0);
	TCCR1A |= ((COM1A & 0x02) << (COM1A1 - 1));
	//Set CS1 to divide the clock by 256
	TCCR1B |= ((CS1 & 0x01) << CS10);
	TCCR1B |= ((CS1 & 0x02) << (CS11 - 1));
	TCCR1B |= ((CS1 & 0x04) << (CS12 - 2));

	//Set OCR1A to be 46875*256 = 12000000 clock cycles at 12 MHz = 1 second
	OCR1A = 46875;
	return;
}

void colours(void){
	red ++;
	//Start fading green in after red is bright
	if (red > 160){
		green ++;
	}
	//Start fading blue in after red is on and green is bright
	if (green > 180){
		blue ++;
	}
	brightness(red, green, blue);
	return;
}

void brightness(uint8_t redBrightness, uint8_t greenBrightness, uint8_t blueBrightness){
	//Modify the compare match outputs for RGB to get PWM
	OCR2A = redBrightness;
	OCR2B = blueBrightness;
	OCR0A = greenBrightness;
	return;
}
