/*
* All rights reserved
* ozforester (www.cqham.ru/forum)
* MIT license
*/

#include "main.h"

#define ETPS8 TIM1->SMCR &= ~TIM_SMCR_ETPS ;  TIM1->SMCR |= TIM_SMCR_ETPS_1|TIM_SMCR_ETPS_0 ; // etps 8
#define ETPS4 TIM1->SMCR &= ~TIM_SMCR_ETPS ;  TIM1->SMCR |= TIM_SMCR_ETPS_1 ; // etps 4
#define ETPS2 TIM1->SMCR &= ~TIM_SMCR_ETPS ;  TIM1->SMCR |= TIM_SMCR_ETPS_0 ; // etps 2
#define ETPS1 TIM1->SMCR &= ~TIM_SMCR_ETPS ; // etps 1

/*
*           G L O B A L
*/

volatile uint32_t EtrFreq, EtrOld ;
volatile uint8_t DataReady = 0;
char str[17] ;
volatile uint8_t etps, etpsold ;

/*
*              M A I N
*/

int main()
{
clock_init();
bias_init();
hd44780_init();
hd44780_string("ozforester");
delay_ms(270);
etr_init();
while(1)
if( DataReady )
	{
	led_toggle();
        etps = 1 ;
        for( volatile uint8_t i = 0  ; i < ((TIM1->SMCR&TIM_SMCR_ETPS)>>12) ; i++ ) etps*=2 ;
        EtrFreq *= etps ;
	if( EtrFreq != EtrOld || etpsold != etps )
		{
		etpsold = etps ;
		EtrOld = EtrFreq ;
		hd44780_clear();
		hd44780_pos(0, 0);
		itoa((int)EtrFreq, str, 10) ;
		hd44780_string(str);
		itoa( (int)etps , str, 10 ) ;
                hd44780_pos(1, 0);
                hd44780_string("etps ");
                hd44780_string(str);
		}
        if(EtrFreq < 12500000){ ETPS1 ; }
        else if(EtrFreq < 25000000){ ETPS2 ; }
        else if(EtrFreq < 50000000){ ETPS4 ; }
        else{ ETPS8 ; }
	DataReady = 0 ;
	}
}

/*
*           G A T E  &  E T R
*/

void etr_init( void ){ // TIM1 ETR in PA12 AF2, TIM1_CH2 PA9 (19) AF2
  // TIM3 Slave
  RCC->APB1ENR  |= RCC_APB1ENR_TIM3EN; // clocking TIM3
  TIM3->SMCR 	|= TIM_SMCR_SMS_2|TIM_SMCR_SMS_1|TIM_SMCR_SMS_0 ; // clocked by master TIM1
  TIM3->CR1  	|= TIM_CR1_CEN ; // enable
  // TIM1 Master, Etr, TI2 Gated
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock port A
  GPIOA->MODER &= ~GPIO_MODER_MODER12; // reset PA12
  GPIOA->MODER |= GPIO_MODER_MODER12_1; // PA12 AF
  GPIOA->AFR[1] |= ( 0b0010 << GPIO_AFRH_AFSEL12_Pos ); // PA12 AF2
  GPIOA->MODER &= ~GPIO_MODER_MODER9; // reset PA9
  GPIOA->MODER |= GPIO_MODER_MODER9_1; // PA9 AF
  GPIOA->AFR[1] |= ( 0b0010 << GPIO_AFRH_AFSEL9_Pos ); // PA9 AF2
  RCC->APB2ENR 	|= RCC_APB2ENR_TIM1EN; // clocking TIM1
  TIM1->SMCR	|= TIM_SMCR_ECE; //ext. clock mode2 enabled, counter is clocked by ETRF signal
  TIM1->SMCR    |= TIM_SMCR_ETPS_1|TIM_SMCR_ETPS_0 ; // etr prescaler 8
  TIM1->CCMR1   |= TIM_CCMR1_CC2S_0 ; // map IC2 TI2FP2 to TI2
  TIM1->SMCR    |= TIM_SMCR_SMS_2|TIM_SMCR_SMS_0 ; // SMS=101 gated
  TIM1->SMCR	|= TIM_SMCR_TS_2|TIM_SMCR_TS_1 ; // TS=110 ti2fp2
  TIM1->CR2        |= TIM_CR2_MMS_1 ; // updates its slave
  TIM1->CR1        |= TIM_CR1_CEN ; // enable
 // TIM17 Gate CC1 CH1 PA7 (13) AF5
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock port A
  GPIOA->MODER &= ~GPIO_MODER_MODER7; // reset PA7
  GPIOA->MODER |= GPIO_MODER_MODER7_1; // PA7 AF
  GPIOA->AFR[0] |= ( 0b0101 << GPIO_AFRL_AFSEL7_Pos ); // PA7 AF5
  RCC->APB2ENR    |= RCC_APB2ENR_TIM17EN; // clocking
  TIM17->PSC      = 50000 - 1 ;
  TIM17->ARR      = 1200 - 1 ;
  TIM17->CCR1     = 1000 ; // duty
  TIM17->CCMR1    |= TIM_CCMR1_OC1M_2|TIM_CCMR1_OC1M_1|TIM_CCMR1_OC1M_0 ; // pwm mode 2
  TIM17->CCER     |= TIM_CCER_CC1E|TIM_CCER_CC1P ;    // CC1 output enable
  TIM17->BDTR	  |= TIM_BDTR_MOE ; // moe-bit
  TIM17->DIER     |= TIM_DIER_CC1IE ; // enable interrupt
  TIM17->CR1    |= TIM_CR1_CEN; // counter enable
  NVIC_EnableIRQ(TIM17_IRQn);
}

void TIM17_IRQHandler(void){
        TIM17->SR &= ~TIM_SR_CC1IF; // reset interrupt flag
	EtrFreq = TIM1->CNT | (TIM3->CNT << 16) ;
        TIM1->CNT = 0 ;
        TIM3->CNT = 0 ;
        DataReady = 1 ; // Etr variable is ready
}


/*
*           H D 4 4 7 8 0
*
* RS  - PA1 -> (W)
* RW  - PA2 ->
* E   - PA3 ->
* D4  - PB4 <-> (R/W)
* D5  - PB5 <->
* D6  - PB6 <->
* D7  - PB7 <->
*
*/

// control
#define RSu GPIOA->MODER |= GPIO_MODER_MODER1_0; GPIOA->BSRR  |= GPIO_BSRR_BS_1;
#define RSd GPIOA->MODER |= GPIO_MODER_MODER1_0; GPIOA->BSRR  |= GPIO_BSRR_BR_1;
#define RWu GPIOA->MODER |= GPIO_MODER_MODER2_0; GPIOA->BSRR  |= GPIO_BSRR_BS_2;
#define RWd GPIOA->MODER |= GPIO_MODER_MODER2_0; GPIOA->BSRR  |= GPIO_BSRR_BR_2;
#define Eu GPIOA->MODER |= GPIO_MODER_MODER3_0; GPIOA->BSRR  |= GPIO_BSRR_BS_3;
#define Ed GPIOA->MODER |= GPIO_MODER_MODER3_0; GPIOA->BSRR  |= GPIO_BSRR_BR_3;
// data
#define D4u GPIOB->MODER |= GPIO_MODER_MODER4_0; GPIOB->BSRR  |= GPIO_BSRR_BS_4;
#define D4d GPIOB->MODER |= GPIO_MODER_MODER4_0; GPIOB->BSRR  |= GPIO_BSRR_BR_4;
#define D5u GPIOB->MODER |= GPIO_MODER_MODER5_0; GPIOB->BSRR  |= GPIO_BSRR_BS_5;
#define D5d GPIOB->MODER |= GPIO_MODER_MODER5_0; GPIOB->BSRR  |= GPIO_BSRR_BR_5;
#define D6u GPIOB->MODER |= GPIO_MODER_MODER6_0; GPIOB->BSRR  |= GPIO_BSRR_BS_6;
#define D6d GPIOB->MODER |= GPIO_MODER_MODER6_0; GPIOB->BSRR  |= GPIO_BSRR_BR_6;
#define D7u GPIOB->MODER |= GPIO_MODER_MODER7_0; GPIOB->BSRR  |= GPIO_BSRR_BS_7;
#define D7d GPIOB->MODER |= GPIO_MODER_MODER7_0; GPIOB->BSRR  |= GPIO_BSRR_BR_7;
#define Dz GPIOB->MODER &= 0xffff00ff ;
#define Di ( GPIOB->IDR & GPIO_IDR_7 ) // read d7

int bf( void ){
  volatile uint8_t flag;
  Dz; RSd; RWu;
  Eu; flag = Di; Ed;
  return(flag);
}

void hd44780_nibble( uint8_t data ){
  if(data & 0b1000){ D7u }
  else { D7d }
  if(data & 0b0100){ D6u }
  else { D6d }
  if(data & 0b0010){ D5u }
  else { D5d }
  if(data & 0b0001){ D4u }
  else { D4d }
  delay_us(500);
  Eu;
  delay_us(500);
  Ed;
  delay_us(500);
}

void hd44780_data( uint8_t data ){
        RSu; RWd; Ed; // data
        hd44780_nibble( data );
}

void hd44780_command( uint8_t data ){
        RSd; RWd; Ed; // data
	hd44780_nibble( data );
}

void hd44780_clear( void ){
        hd44780_command(0);
        hd44780_command(1);
}

void hd44780_pos(char y, char x){
        hd44780_command( (0x80 | (0x40*y+x)) >> 4 ); // cgram
	hd44780_command( (0x80 | (0x40*y+x)) & 0xf );
}

void hd44780_lshift(void){
        hd44780_command(1);
        hd44780_command(8);
}

void hd44780_rshift(void){
        hd44780_command(1);
        hd44780_command(12);
}

void hd44780_char( char ch){
	hd44780_data( ( ch & 0xf0 ) >> 4 ); // high nibble
        hd44780_data( ( ch & 0x0f ) ); // low nibble
}

void hd44780_string(char *str){
	do{
	  hd44780_char(*str);
	  } while ( *(++str) != 0 );
}

void hd44780_init(void){
RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock port A
RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // clock port B
  RSd; RWd; Ed; // commands
  delay_ms(15); // initialization
  hd44780_command(3);  delay_ms(5);
  hd44780_command(3);  delay_us(100); hd44780_command(3);
  hd44780_command(2);  while(bf());
  hd44780_command(2); // function set 4-bit, 2-lines, 5x8 dots
  hd44780_command(8);  while(bf());
  hd44780_command(0);  // display off, cursor off, blinking off
  hd44780_command(8);  while(bf());
  hd44780_command(0); // clear display
  hd44780_command(1);  while(bf());
  hd44780_command(0); // entry mode set: increment ddram w/o shift
  hd44780_command(6);  while(bf());
  hd44780_command(0); // display on w/o cursor w/o blinking
  hd44780_command(12);  while(bf());
}

/*
*               L C D   B I A S
*/

void bias_init( void ){ // pwm tim14 ch1 af4 pa4
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock port
  GPIOA->MODER &= ~GPIO_MODER_MODER4; // reset
  GPIOA->MODER |= GPIO_MODER_MODER4_1; // AF
  GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR4_1|GPIO_OSPEEDR_OSPEEDR4_0; // HIGH SPEED
  GPIOA->AFR[0] |= 4 << GPIO_AFRL_AFSEL4_Pos; // PA4 AF4
  RCC->APB1ENR    |= RCC_APB1ENR_TIM14EN; // clocking TIM
  TIM14->PSC        = 50 - 1;
  TIM14->ARR        = 100 - 1;
  TIM14->CCR1       = 6  ; // duty -300..400 mV
  TIM14->CCMR1      |= TIM_CCMR1_OC1M_2|TIM_CCMR1_OC1M_1|TIM_CCMR1_OC1M_0 ; // pwm mode 2
  TIM14->CCER       |= TIM_CCER_CC1E ;    // CC1 output enable
  TIM14->CR1        |= TIM_CR1_CEN ; // enable timer
}

/*
*               R C C
*/

void clock_init(void){ // HSE PLL 50 Mhz -------- MCO PA8 (18) AF AF0
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock port
  GPIOA->MODER &= ~GPIO_MODER_MODER8; // reset
  GPIOA->MODER |= GPIO_MODER_MODER8_1; // AF
  GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR8_1|GPIO_OSPEEDR_OSPEEDR8_0; // HIGH SPEED
  RCC->CR |= RCC_CR_HSEON ;
  while( !( RCC->CR & RCC_CR_HSERDY ) ) ; // wait hse ready
  RCC->CR &= ~RCC_CR_PLLON ;
  while( RCC->CR & RCC_CR_PLLRDY );
  RCC->CFGR |= RCC_CFGR_PLLSRC; // PLL src HSE (25 MHz)
  RCC->CFGR2 &= ~RCC_CFGR2_PREDIV ; // reset
  RCC->CFGR2 |= RCC_CFGR2_PREDIV_2 ; // /5 (5MHz)
  RCC->CFGR &= ~RCC_CFGR_PLLMUL ; // reset
  RCC->CFGR |= RCC_CFGR_PLLMUL_3 ; // x10 (50 MHz)
  RCC->CR |= RCC_CR_PLLON ;
  while( !(RCC->CR & RCC_CR_PLLRDY ) );
  FLASH->ACR |= FLASH_ACR_LATENCY ;
  RCC->CFGR |= RCC_CFGR_SW_1 ;
  while ( !(RCC->CFGR & RCC_CFGR_SWS_1 ) );
  RCC->CFGR |= RCC->CFGR |= RCC_CFGR_MCO_PLL|RCC_CFGR_PLLNODIV|RCC_CFGR_MCOPRE_DIV1 ; // MCO
}


/*
*             U T I L I T I E S
*/

void led_on( void ){
RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock port
GPIOA->MODER |= GPIO_MODER_MODER5_0; // out
GPIOA->MODER |= GPIO_MODER_MODER6_0; // out
GPIOA->BSRR  |= GPIO_BSRR_BS_5;
GPIOA->BSRR  |= GPIO_BSRR_BR_6;
}

void led_off( void ){
RCC->AHBENR |= RCC_AHBENR_GPIOAEN; // clock port
GPIOA->MODER |= GPIO_MODER_MODER5_0; // out
GPIOA->MODER |= GPIO_MODER_MODER6_0; // out
GPIOA->BSRR  |= GPIO_BSRR_BR_5;
GPIOA->BSRR  |= GPIO_BSRR_BS_6;
}

void led_toggle(void)
{
if( GPIOA->IDR & GPIO_IDR_5 ) led_off() ;
else led_on() ;
}

void delay_ms( volatile uint32_t d ){
  d *= 5000 ;
  for( volatile uint32_t i = 0; i < d; i++ ){}
}

void delay_us( volatile uint32_t d ){
  d *= 5 ;
  for( volatile uint32_t i = 0; i < d; i++ ){}
}

/* EOF */
