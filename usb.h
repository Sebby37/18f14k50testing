/* 
 * File:   usb.h
 * Author: sebc
 *
 * Created on 27 April 2026, 2:02 PM
 */

#include <xc.h>
#include <string.h>

#ifndef USB_H
#define	USB_H

#ifdef	__cplusplus
extern "C" {
#endif
// =============================================================================
typedef unsigned char u8;
typedef unsigned short u16;

#define USBBD_start 0x200
#define USBDATA_START 0x280

extern volatile u8 BD0STAT  __at(0x200);
extern volatile u8 BD0CNT   __at(0x201);
extern volatile u8 BD0ADRL  __at(0x202);
extern volatile u8 BD0ADRH  __at(0x203);
#define EP0DATA 0x280

extern volatile u8 BD1STAT  __at(0x204);
extern volatile u8 BD1CNT   __at(0x205);
extern volatile u8 BD1ADRL  __at(0x206);
extern volatile u8 BD1ADRH  __at(0x207);
#define EP1DATA 0x2C0

struct control_transfer {
    union {
        u8 bitmap;
        struct {
            unsigned recipient  :5;
            unsigned type       :2;
            unsigned direction  :1;
        };
    } bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
};

// Standard SETUP requests
#define GET_STATUS 0x00
#define CLEAR_FEATURE 0x01
#define SET_FEATURE 0x03
#define SET_ADDRESS 0x05
#define GET_DESCRIPTOR 0x06
#define SET_DESCRIPTOR 0x07
#define GET_CONFIGURATION 0x08
#define SET_CONFIGURATION 0x09

// Descriptor types
#define DESC_DEVICE 1
#define DESC_CONFIGURATION 2
#define DESC_STRING 3
#define DESC_INTERFACE 4

static struct desc_device {
    u8 bLength;
    u8 bDescriptorType;
    u16 bcdUSB;
    u8 bDeviceClass;
    u8 bDeviceSubClass;
    u8 bDeviceProtocol;
    u8 bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8 iManufacturer;
    u8 iProduct;
    u8 iSerialNumber;
    u8 bNumConfigurations;
} device_descriptor = {
    18,             // Length
    DESC_DEVICE,    // Descriptor Type
    0x0200,         // USB version (2.0)
    0xFF,           // Device class (vendor-specific)
    0x00,           // Subclass code
    0x00,           // Device protocol
    64,             // Max packet size
    0x1209,         // VID (pid.codes one)
    0x8310,         // PID (made up)
    0x1234,         // Device BCD revision (made up)
    0,0,0,          // No string descriptors
    0x01            // Device configurations
};

static struct desc_config {
    u8 bLength;
    u8 bDescriptorType;
    u16 wTotalLength;
    u8 bNumInterfaces;
    u8 bConfigurationValue;
    u8 iConfiguration;
    union {
        u8 attributes;
        struct {
            unsigned                :5;
            unsigned remoteWakeup   :1;
            unsigned selfPowered    :1;
            unsigned                :1;
        };
    } bmAttributes;
    u8 bMaxPower;
} config_descriptor = {
    9,                  // Config descriptor length
    DESC_CONFIGURATION, // Descriptor type
    18,                 // Total length of configuration (config desc + interface descs + endpoint descs)
    1,                  // Number of interfaces (1)
    1,                  // Configuration "index"
    0,                  // String descriptor (not used lmao)
    0,                  // Attributes, just 0 because I don't use the real ones
    50                  // 100mAh max power consumption (a safe estimate)
};

static struct desc_interface {
    u8 bLength;
    u8 bDescriptorType;
    u8 bInterfaceNumber;
    u8 bAlternateSetting;
    u8 bNumEndpoints;
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
    u8 iInterface;
} interface_descriptor = {
    9,              // Descriptor length
    DESC_INTERFACE, // Descriptor type
    0,              // Zero-based interface number/index
    0,              // Alternate setting (?)
    0,              // Number of endpoints, 0 for us
    0xFF,           // Interface class (vendor specific)
    0x00,           // Subclass, must be set to 0
    0xFF,           // Protocol (vendor specific)
    0               // Interface string descriptor
};

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
    memset((void*)EP0DATA, 0, 64);
    
    // Final configurations
    // Note that the Ping Pong Buffers are fully disabled, since this is by default its not here
    UCFGbits.FSEN = 1;  // ENABLE FULL SPEED YEAHHH
    UCFGbits.UPUEN = 1; // Also enable the on-chip pullups so we don't need our own resistors
    UIE = 0xFF;         // Enable ALL USB interrupts
    UEIE = 0xFF;        // Enable ALL USB ERROR interrupts
    UCONbits.USBEN = 1; // We are ready for USB functionality
}

// Called after a TRNIF interrupt
void usb_event(void)
{
    u8 direction = USTATbits.DIR;
    u8 endpoint = USTATbits.ENDP0; // Since we only have 2 endpoints we only need to poll the 0th bit
    printf("USB Event! USTAT = %X, UCON = %x\n", USTAT, UCON);
    TRNIF = 0;
    
    if (endpoint == 0 && PKTDIS) {
        // This be a control transfer, meaning all transfers are disabled until PKTDIS is cleared
        struct control_transfer* setup_pkt = ((struct control_transfer*)EP0DATA);
        switch (setup_pkt->bRequest) {
            case GET_DESCRIPTOR: {
                // Pick the right descriptor
                void *desc;
                switch (setup_pkt->wValue) {
                    case DESC_DEVICE:
                        desc = &device_descriptor; break;
                    case DESC_CONFIGURATION:
                        desc = &config_descriptor; break;
                    case DESC_INTERFACE:
                        desc = &interface_descriptor; break;
                }
                // Put it into the EP0 buffer
                memcpy((void*)EP0DATA, desc, setup_pkt->wLength);
            } break;
            case SET_ADDRESS: {
                // Copy over the address to the address-specific register
                UADDR = (u8)setup_pkt->wValue;
            } break;
            default: {
                // Stall on invalid/unknown request
                UEP0bits.EPSTALL = 1;
            } break;
        }
        
        BD0STAT |= (1 << 7); // Arm the buffer
        PKTDIS = 0; // Re-enable packet processing
    } else if (endpoint == 0 && !PKTDIS) {
        BD0STAT |= (1 << 7); // Arm the buffer, it no matter :3
    }
}

// =============================================================================
#ifdef	__cplusplus
}
#endif

#endif	/* USB_H */

