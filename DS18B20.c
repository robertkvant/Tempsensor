//
//  DS18B20.c
//  
//
//  Created by robert on 2015-01-27.
//
//

#include <avr/io.h>
#include <util/delay.h>
#include "DS18B20.h"

// -------------Miscellaneous macros------------------
#define PULL_BUS_LOW    (DDRF = DDRF | 0b00000001)
#define RELEASE_BUS     (DDRF = DDRF & 0b11111110)

#define BUS_IS_HIGH     ((PINF & 0b00000001) == 0b00000001)
#define BUS_IS_LOW      ((PINF & 0b00000001) == 0b00000000)

#define READ_BUS        (PINF & 0b00000001)

// ROM-commands
#define SKIP_ROM        issueCommand(0xCC)

// Function-commands
#define CONVERT_T       {issueCommand(0x44); while (readTimeSlot() == 0){}}
#define READ_SCRATCH    issueCommand(0xBE)


/* -------------Scratchpad structure--------------
 Contains all fields in scratchpad-register (DS18B20)
 */
struct ScratchPad{
    
    unsigned int  temperature;
    unsigned char th_register;
    unsigned char tl_register;
    unsigned char conf_register;
    unsigned char reserved_one;
    unsigned char reserved_two;
    unsigned char reserved_three;
    unsigned char crc;
    
    
} scratchPad;



/*  INITIALIZATION PROCEDURE—RESET AND PRESENCE PULSES
 --------------------------------------------------
 'All communication with the DS18B20 begins with an initialization
 sequence that consists of a reset pulse from the master followed 
 by a presence pulse from the DS18B20.'
 
 */

void initSequence(void){
    
    // PORTF0 always zero.
    PORTF &= 0b11111110;
    
    
    // ---- MASTER TX RESET PULSE ---- //
    
    PULL_BUS_LOW;
    
    _delay_us(480);
    
    RELEASE_BUS;
    
   
    // ---- MASTER RX (SENSOR PRESENCE PULSE) ---- //
    
    _delay_us(15);
 
    while (BUS_IS_HIGH) {}
    
    _delay_us(60);

    while (BUS_IS_LOW) {}
    
    _delay_us(405);
    
}



/*  WRITE TIME SLOT
 -----------------------------
 'There are two types of write time slots: “Write 1” time slots
 and “Write 0” time slots. The bus master uses a Write 1 time
 slot to write a logic 1 to the DS18B20 and a Write 0 time slot
 to write a logic 0 to the DS18B20. All write time slots must
 be a minimum of 60μs in duration with a minimum of a 1μs recovery
 time between individual write slots. Both types of write time slots
 are initiated by the master pulling the 1-Wire bus low.'
 
 void writeTimeSlot(char bit) takes zero or one as input and writes
 it to the sensor.
 
 */

void writeTimeSlot(unsigned char bit){
    
    switch (bit) {
            
            
        case 0x00:
            
            PULL_BUS_LOW;
            
            _delay_us(60);
            
            RELEASE_BUS;
            
            break;
            
        case 0x01:
            
            PULL_BUS_LOW;;
            
            _delay_us(10);
            
            RELEASE_BUS;
            
            _delay_us(50);
            
            break;

    }
    
    _delay_us(2);
    
}

/* READ TIME SLOTS
 -------------------------------
 'The DS18B20 can only transmit data to the master when the master
 issues read time slots. Therefore, the master must generate read
 time slots immediately after issuing a Read Scratchpad [BEh] or
 Read Power Supply [B4h] command, so that the DS18B20 can provide
 the requested data.'
 
 unsigned char readTimeSlot() returns the read bit.
 
 */

unsigned char readTimeSlot(void){
    
    unsigned char readBit = 0;
    
    // --- Master initiates the read time slot --
    
    PULL_BUS_LOW;
    
    _delay_us(2);
    
    RELEASE_BUS;
    
    _delay_us(10);
    
    //Sample bus
    readBit = READ_BUS;
    
    _delay_us(48);
    
    while (BUS_IS_LOW) {}
    
    _delay_us(2);    

    return readBit;
    
}


void issueCommand(unsigned char command){
    
    unsigned char mask = 0x01;
    
    for (unsigned char i = 0; i < 8; i++){
        
        writeTimeSlot((mask & command) >> i);
        
        mask <<= 1;
    }
    
}


unsigned int readScratchPad(void){
    
    scratchPad = (struct ScratchPad){0};
    
    initSequence();
    
    SKIP_ROM;
    CONVERT_T;
    
    initSequence();
    
    SKIP_ROM;
    READ_SCRATCH;
    
    
    // Read temperature
    for (unsigned char i = 0; i < 16; i++) {
        
        if(readTimeSlot()){
            scratchPad.temperature |= (1 << i);
        }
        
    }
    
    // Read Th-Register
    for (unsigned char i = 0; i < 8; i++) {
        
        if(readTimeSlot()){
            scratchPad.th_register |= (1 << i);
        }
        
    }
    
    // Read Tl-Register
    for (unsigned char i = 0; i < 8; i++) {
        
        if(readTimeSlot()){
            scratchPad.tl_register |= (1 << i);
        }
        
    }
    
    // Read Configuration-Register
    for (unsigned char i = 0; i < 8; i++) {
        
        if(readTimeSlot()){
            scratchPad.conf_register |= (1 << i);
        }
        
    }
    
    // Read Reserved Bytes 1
    for (unsigned char i = 0; i < 8; i++) {
        
        if(readTimeSlot()){
            scratchPad.reserved_one |= (1 << i);
        }
        
    }
    
    // Read Reserved Bytes 2
    for (unsigned char i = 0; i < 8; i++) {
        
        if(readTimeSlot()){
            scratchPad.reserved_two |= (1 << i);
        }
        
    }
    
    // Read Reserved Bytes 3
    for (unsigned char i = 0; i < 8; i++) {
        
        if(readTimeSlot()){
            scratchPad.reserved_three |= (1 << i);
        }
        
    }
    
    // Read CRC
    for (unsigned char i = 0; i < 8; i++) {
        
        if(readTimeSlot()){
            scratchPad.crc |= (1 << i);
        }
        
    }
    
    return 1;
}

unsigned int getTemperatureRegisterData(void){
    return scratchPad.temperature;
}

unsigned int getCrcRegisterData(void){
    return scratchPad.crc;
}
