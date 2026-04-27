/*
 * File:   main.c
 * Author: SebC
 *
 * Created on April 22, 2026, 7:47 PM
 */


#include <xc.h>
#include <stdio.h>
#include "usb.h"

#pragma config CPUDIV=NOCLKDIV  // The clock is NOT being divided
#pragma config USBDIV=OFF       // The USB clock is directly from OSC1/2 with NO divide
#pragma config FOSC=HS          // The oscillator source is an external (high speed) crystal resonator
#pragma config PLLEN=OFF        // The Phase Lock Loop is disabled, letting us run at full 48MHz
#pragma config BORV=19          // Brown-out reset voltage thingie, I cant remember what this is used for lmao
#define _XTAL_FREQ  48000000    // We are running at 48MHz clock speed

void __interrupt(high_priority) tcInt(void)
{
    printf("Interrupt! UIR=%x UEIR=%x\n", UIR, UEIR);
//    UIR = 0;
//    UIE = 0;
    if (TMR0IE && TMR0IF) {
        TMR0IF = 0;
        LATBbits.LB4 = ~LATBbits.LB4;
//        TMR0 = 0x0BDC;
//        printf("Hey! The timer1 is %hu\n", TMR1);
    }
    return;
}

// An override function that allows any print/put function to work
// In this case, such functions will "print" to the serial out port
void putch(char data)
{
    // Wait for the transmit buffer to become empty, then "enqueue" the byte
    while (!TRMT);
    TXREG = data;
}

void uart_init(void)
{
    // Set the baud rate generator to generate approx 9600 baud
    SPBRG = 18; // Calculated precisely for a 12MHz clock signal
    SPBRGH = 0;
    BRGH = 0; // Not high-speed transmission
    SYNC = 0; // Asynchronous mode
    SPEN = 1; // Enable the serial port
    TXEN = 1; // Enable transmission
}

void main(void) 
{
    // Disable ALL ADC lines
    ANSEL = 0;
    ANSELH = 0;
    
    // Set Port B to be all outputs
    TRISB = 0;
    
    // Enable interrupts, peripheral ones and the Timer0 interrupt on overflow
    // Can check if the interrupt was TMR0 overflow via the TMR0IF bit
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.TMR0IE = 1;
    
    // Timer0 enabled, is 16 bit, transitions on the internal instruction cycle, increments on low-high, assigned the prescaler and runs at 1:32 prescale
    // This effectively runs it at ~500ms intervals since we have a 1MHz clock
    T0CON = 0b10000100;
    
    T1CON = 0b1000100;
    
    uart_init();
    puts("Hello from the chip!");
    
    __delay_ms(250);
    usb_init();
    
    while (1) {
        while (!TRNIF);
        usb_event();
    }
    
    return;
}
