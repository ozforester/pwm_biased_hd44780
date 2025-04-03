#ifndef _MAIN_H_
#define _MAIN_H_

#include <stm32f0xx.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void pwm_init( void );
void etr_init( void );
void hd44780_init(void);
void hd44780_nibble( uint8_t );
void hd44780_data( uint8_t );
void hd44780_command( uint8_t);
void hd44780_clear( void );
void hd44780_pos(char, char);
void hd44780_lshift(void);
void hd44780_rshift(void);
void hd44780_char(char );
void hd44780_string(char*);
void bias_init( void );
void clock_init(void);
void delay_ms( volatile uint32_t );
void delay_us( volatile uint32_t );
void si_init(void);
void twi_init1(void);
void twi_write_byte1( uint8_t);
void tdelay( volatile uint32_t );
void i2c_write( volatile uint8_t  , volatile uint8_t * , volatile uint8_t);

#endif
