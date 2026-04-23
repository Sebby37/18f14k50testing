/*
 * File:   main.c
 * Author: SebC
 *
 * Created on April 22, 2026, 7:47 PM
 */


#include <xc.h>
#include <stdio.h>

#pragma config CPUDIV=CLKDIV4
#pragma config USBDIV=ON
#pragma config FOSC=HS
#pragma config PLLEN=ON
#pragma config BORV=19
#define _XTAL_FREQ  12000000

void __interrupt(high_priority) tcInt(void)
{
    if (TMR0IE && TMR0IF) {
        TMR0IF = 0;
        LATBbits.LB4 = ~LATBbits.LB4;
//        TMR0 = 0x0BDC;
        printf("Hey! The timer1 is %hu\n", TMR1);
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
    ANSEL = 0;
    ANSELH = 0;
    TRISB = 0;
    OSCCONbits.IRCF = 0b111; // 16MHz clock prescaler
    
    // Enable interrupts, peripheral ones and the Timer0 interrupt on overflow
    // Can check if the interrupt was TMR0 overflow via the TMR0IF bit
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.TMR0IE = 1;
    
    // Timer0 enabled, is 16 bit, transitions on the internal instruction cycle, increments on low-high, assigned the prescaler and runs at 1:32 prescale
    // This effectively runs it at ~500ms intervals since we have a 1MHz clock
    T0CON = 0b10000100;
    
    T1CON = 0b10001011;
    
    uart_init();
    puts("Hello from the chip!");
    
    while (1);
    
    return;
}
