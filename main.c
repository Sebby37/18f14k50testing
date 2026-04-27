/*
 * File:   main.c
 * Author: SebC
 *
 * Created on April 22, 2026, 7:47 PM
 */


#include <xc.h>
#include <stdio.h>
#include <stdint.h>

#pragma config CPUDIV=NOCLKDIV  // The clock is NOT being divided
#pragma config USBDIV=OFF       // The USB clock is directly from OSC1/2 with NO divide
#pragma config FOSC=HS          // The oscillator source is an external (high speed) crystal resonator
#pragma config PLLEN=OFF        // The Phase Lock Loop is disabled, letting us run at full 48MHz
#pragma config BORV=19          // Brown-out reset voltage thingie, I cant remember what this is used for lmao
#define _XTAL_FREQ  48000000    // We are running at 48MHz clock speed

#define USBBD_start 0x200
#define USBDATA_START 0x280

extern volatile unsigned char BD0STAT  __at(0x200);
extern volatile unsigned char BD0CNT   __at(0x201);
extern volatile unsigned char BD0ADRL  __at(0x202);
extern volatile unsigned char BD0ADRH  __at(0x203);
#define EP0DATA 0x280

extern volatile unsigned char BD1STAT  __at(0x204);
extern volatile unsigned char BD1CNT   __at(0x205);
extern volatile unsigned char BD1ADRL  __at(0x206);
extern volatile unsigned char BD1ADRH  __at(0x207);
#define EP1DATA 0x2C0

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

void usb_init(void)
{
    // Setup endpoint 0, aka the control endpoint
    UEP0bits.EPHSHK = 1;    // Enable handshaking
    UEP0bits.EPCONDIS = 0;  // Enable control transfers
    UEP0bits.EPOUTEN = 1;   // Enable OUT transactions
    UEP0bits.EPINEN = 1;    // Enable IN transactions
    // And now its buffer descriptors
    BD0CNT = 64;    // Full speed baby!
    BD0ADRL = 0x80;
    BD0ADRH = 0x02;
    BD0STAT = 0;    // All bits here can be 0'd
    
    // Setup EP1, the HID endpoint
    UEP1bits.EP1HSHK = 1;   // Handshaking
    UEP1bits.EPCONDIS = 1;  // No control transfers
    UEP1bits.EPOUTEN = 0;   // No OUT transactions
    UEP1bits.EPINEN = 1;    // Enable IN transactions
    // BD too
    BD1CNT = 1;     // The HID report is 1 byte (6 bits technically) so thats all I'll use
    BD1ADRL = 0xC0;
    BD1ADRH = 0x02;
    BD1STAT = 0;
    
    // Final configurations
    // Note that the Ping Pong Buffers are fully disabled, since this is by default its not here
    UCFGbits.FSEN = 1;  // ENABLE FULL SPEED YEAHHH
    UCFGbits.UPUEN = 1; // Also enable the on-chip pullups so we don't need our own resistors
    UCONbits.USBEN = 1; // We are ready for USB functionality
}

void usb_loop(void)
{
    
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
    
    while (1);
    
    return;
}
