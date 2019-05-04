#include "VGAXUA.h"

#define MSPIMSCK 4

#if defined(__AVR_ATmega2560__)
  //HSYNC pin used by TIMER2
  #define HSYNCPIN 9 
  //VSYNC pin used by TIMER1
  #define VSYNCPIN 11
  //These two pin cannot be modified without modify the HSYNC assembler code
  #define COLORPIN0 30
  #define COLORPIN1 31
  #define EXTRACOLORPORT PORTC
#else
  //HSYNC pin used by TIMER2
  #define HSYNCPIN 3
  //VSYNC pin used by TIMER1
  #define VSYNCPIN 9
  //These two pin cannot be modified without modify the HSYNC assembler code
  #define COLORPIN0 6
  #define COLORPIN1 7
  #define EXTRACOLORPORT PORTD
#endif

//Number of VGA lines to be skipped (black lines)
/*These lines includes the vertical sync pulse and back porch.
Minimum value must be 35 (calculate from Nick Gammon)
You can modify this value to center the framebuffer vertically, or not*/
#define SKIPLINES 35

unsigned vtimer;
static byte aline, rlinecnt;
static byte vskip;
byte vgaxfb[VGAX_HEIGHT*VGAX_BWIDTH];
static byte *videoline;
static byte vmask;

//VSYNC interrupt
ISR(TIMER1_OVF_vect) {
  aline=-1;
  vskip=SKIPLINES;
  vtimer++;
  rlinecnt=0;
  videoline=vgaxfb;
}
//HSYNC interrupt
ISR(TIMER2_OVF_vect) {
  /*
  NOTE: I prefer to generate the line here, inside the interrupt.
  Gammon's code generate the line pixels inside main().
  My versin generate the signal using only interrupts, so inside main() function
  you can do anything you want. Your code will be interrupted when VGA signal
  needs to be generated
  */
  //check vertical porch
  if (vskip) {
      vskip--;
      return;
  }
  if (rlinecnt<VGAX_HEIGHT) { 
    #define nop asm volatile("nop")
    #if defined(__AVR_ATmega2560__)
    nop;
    nop;
    #endif

    //interrupt jitter fix (needed to keep signal stable)
    //code from https://github.com/cnlohr/avrcraft/tree/master/terminal
    //modified from VGAX dejitter version (this is faster)    
    #define DEJITTER_SYNC -0 //was -3 on ATMEGA2560
    asm volatile(
      "     lds r16, %[timer0]    \n\t" //
      "     subi r16, %[tsync]    \n\t" //
      "     andi r16, 7           \n\t" //
      "     call TL               \n\t" //
      "TL:                        \n\t" //
      #if defined(__AVR_ATmega2560__)
      "     pop r17               \n\t" //ATMEGA2560 has a 22bit PC!
      #endif
      "     pop r31               \n\t" //
      "     pop r30               \n\t" //
      "     adiw r30, (LW-TL-5)   \n\t" //
      "     add r30, r16          \n\t" //
      "     ijmp                  \n\t" //
      "LW:                        \n\t" //
      "     nop                   \n\t" //
      "     nop                   \n\t" //
      "     nop                   \n\t" //
      "     nop                   \n\t" //
      "     nop                   \n\t" //
      "LBEND:                     \n\t" //
    :
    : [timer0] "i" (&TCNT0),
      [tsync] "i" ((uint8_t)DEJITTER_SYNC)
    : "r30", "r31", "r16", "r17");
    
    register byte *p=videoline;
    register byte c=*p++;;
  
    //first 8px will turn on VMASK (for additional colors combinations)
    #define drawF UDR0=c; UCSR0B=bit(TXEN0); c=*p++; nop;

    //second 8px
    #define drawS UDR0=c; EXTRACOLORPORT=vmask; nop; nop; nop; nop; nop; nop; nop; c=*p++;

    //draw 8px
    #define draw8 UDR0=c; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; c=*p++;

    #if !defined(__AVR_ATmega2560__)
    nop; nop;
    #endif

    //draw 192px
    #if defined(__AVR_ATmega2560__)
    drawF draw8 drawS draw8 
    #else
    drawF drawS draw8 draw8 
    #endif
    draw8 draw8 draw8 draw8
    draw8 draw8 draw8 draw8 
    draw8 draw8 draw8 draw8
    draw8 draw8 draw8 draw8 
    draw8 draw8 draw8 

    #if defined(__AVR_ATmega2560__)
    draw8
    #endif
 
    //draw last 8px
    UDR0=c; 
    UCSR0B=0;

    //wait to disable EXTRACOLORS with the right timing
    nop;
    #if !defined(__AVR_ATmega2560__)
    nop; nop; nop; nop;
    nop; nop; nop; nop;
    nop; nop; nop; nop;
    nop; nop; nop; nop;
    #endif

    //disable VMASK
    EXTRACOLORPORT&=~vmask;
  } 
  //increment framebuffer line counter after 6 VGA lines
  #if defined(__AVR_ATmega2560__) && defined(ATMEGA2560_MAXRES)
    #define CLONED_LINES (2-1)
  #else
    #define CLONED_LINES (6-1)
  #endif
  if (++aline==CLONED_LINES) { 
    aline=-1;
    rlinecnt++;
    videoline+=VGAX_BWIDTH;
  } else {
  }
}
void VGAXUA::begin() {
  //Timers setup code, modified version of the Nick Gammon's VGA sketch
  cli();
  //disable TIMER0 interrupt
  TIMSK0=0;
  TCCR0A=0;
  TCCR0B=(1 << CS00); //enable 16MHz counter (used to fix the HSYNC interrupt jitter)
  OCR0A=0;
  OCR0B=0;
  TCNT0=0;
  //TIMER1 - vertical sync pulses
  pinMode(VSYNCPIN, OUTPUT);
  #if VSYNCPIN==10 //ATMEGA328 PIN 10
  TCCR1A=bit(WGM10) | bit(WGM11) | bit(COM1B1);
  TCCR1B=bit(WGM12) | bit(WGM13) | bit(CS12) | bit(CS10); //1024 prescaler
  OCR1A=259; //16666 / 64 uS=260 (less one)
  OCR1B=0; //64 / 64 uS=1 (less one)
  TIFR1=bit(TOV1); //clear overflow flag
  TIMSK1=bit(TOIE1); //interrupt on overflow on TIMER1
  #else //ATMEGA328 PIN 9 or ATMEGA2560 PIN 11
  TCCR1A=bit(WGM11) | bit(COM1A1);
  TCCR1B=bit(WGM12) | bit(WGM13) | bit(CS12) | bit(CS10); //1024 prescaler
  ICR1=259; //16666 / 64 uS=260 (less one)
  OCR1A=0; //64 / 64 uS=1 (less one)
  TIFR1=bit(TOV1); //clear overflow flag
  TIMSK1=bit(TOIE1); //interrupt on overflow on TIMER1
  #endif
  //TIMER2 - horizontal sync pulses
  pinMode(HSYNCPIN, OUTPUT);
  TCCR2A=bit(WGM20) | bit(WGM21) | bit(COM2B1); //pin3=COM2B1
  TCCR2B=bit(WGM22) | bit(CS21); //8 prescaler
  OCR2A=63; //32 / 0.5 uS=64 (less one)
  OCR2B=7; //4 / 0.5 uS=8 (less one)
  TIFR2=bit(TOV2); //clear overflow flag
  TIMSK2=bit(TOIE2); //interrupt on overflow on TIMER2
  //Setup USART
  UBRR0=0;
  pinMode(MSPIMSCK, OUTPUT);
  UCSR0B=0;
  UCSR0C=bit(UCPOL0) | bit (UMSEL00) | bit (UMSEL01);
  UCSR0B=0;
  //pins for outputting the colour information
  pinMode(COLORPIN0, OUTPUT);
  pinMode(COLORPIN1, OUTPUT);  
  sei();
}
void VGAXUA::end() {
  //disable TIMER0
  TCCR0A=0;
  TCCR0B=0;
  //disable TIMER1
  TCCR1A=0;
  TCCR1B=0;
  //disable TIMER2
  TCCR2A=0;
  TCCR2B=0;
}
void VGAXUA::clear(byte color) {
  register byte c=color;
  c&=1;    //0000 0001
  c|=c<<1; //0000 0011
  c|=c<<2; //0000 1111
  c|=c<<4; //1111 1111
  unsigned cnt=VGAX_BSIZE;
  byte *o=(byte*)vgaxfb;
  while (cnt--)
    *o++=c;
}
void VGAXUA::copy(byte *src) {
  byte *o=(byte*)vgaxfb;
  unsigned cnt=VGAX_BSIZE;
  while (cnt--)
    *o++=pgm_read_byte(src++);
}
void VGAXUA::setExtendedColorsMask(byte mask) {
  vmask=(mask & 3)<<6;
}
void VGAXUA::delay(int msec) {
  while (msec--) {
    unsigned cnt=16000/32; //TODO: use a more precise way to calculate cnt
    while (cnt--)
      asm volatile("nop\nnop\nnop\nnop\n");
  }
}
static unsigned long rand_next=1;

unsigned VGAXUA::rand() {
  rand_next = rand_next * 1103515245UL + 12345;
  return rand_next+((unsigned)(rand_next / 65536) % 32768);
}
