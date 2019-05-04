/*
VGAXUA Library for Arduino UNO (ATMega328) and Arduino MEGA (ATMega2560)
Source code url: https://github.com/smaffer/vgaxua

192x80px VGA framebuffer with 1 color (or 200x240px on MEGA)

COPYRIGHT (C) 2019 Sandro Maffiodo
smaffer@gmail.com
http://www.sandromaffiodo.com

based on the "VGA color video generation" by Nick Gammon:
  http://www.gammon.com.au/forum/?id=11608.
AVR interrupt dejitter from Charles CNLOHR:
  https://github.com/cnlohr/avrcraft/tree/master/terminal

see https://github.com/smaffer/vgaxua for the library description and for the
hardware wiring.

HERE you can find some inline documentation about the VGAXUA library class 
*/
#ifndef __VGAX_UART_library__
#define __VGAX_UART_library__

#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <avr/pgmspace.h>

//uncomment ATMEGA2560_MAXRES to use 200x240px rectangular pixels
#define ATMEGA2560_MAXRES

#if defined(__AVR_ATmega2560__) && defined(ATMEGA2560_MAXRES)
  #define VGAX_HEIGHT 240 //number of lines
#else
  #define VGAX_HEIGHT 80 //number of lines
#endif
#if defined(__AVR_ATmega2560__)
  #define VGAX_BWIDTH 25 //number of bytes in a row
#else
  #define VGAX_BWIDTH 24 //number of bytes in a row
#endif
#define VGAX_WIDTH (VGAX_BWIDTH*8) //number of pixels in a row
#define VGAX_BSIZE (VGAX_BWIDTH*VGAX_HEIGHT) //size of framebuffer in bytes
#define VGAX_SIZE (VGAX_WIDTH*VGAX_HEIGHT) //size of framebuffer in pixels

//framebuffer. if you want you can write directly to this array. its safe
extern byte vgaxfb[VGAX_HEIGHT*VGAX_BWIDTH];

//Extended Colors SKIP Width
#if defined(__AVR_ATmega2560__)
  #define VGAX_ECSWIDTH 11
#else
  #define VGAX_ECSWIDTH 3
#endif

//clock replacement. this is increment in the VSYNC interrupt, so run at 60Hz
extern unsigned vtimer;

//VGAX class. This is a static class. Multiple instances will not work
class VGAXUA {
public:
  /*
   * begin()
   * end()
   *    NOTES: begin() method reconfigure TIMER0 TIMER1 and TIMER2.
   *    If you need to shutdown this library, you need to call end() and
   *    reconfigure all the three timers by yourself. The lib will not
   *    restore the previous timers configuration
   */
  static void begin();
  static void end();
  /*
   * putpixel(x, y, color)
   *    x: horizontal pixel coordinate. Must be less than VGAX_WIDTH
   *    y: vertical pixel coordinate. Must be less than VGAX_HEIGHT
   *    color: 1bit color. you must use only 0 or 1
   */
  static inline void putpixel(byte x, byte y, byte color) {
    byte *p=vgaxfb + y*VGAX_BWIDTH + (x>>3);
    byte bitpos=7-(x & 7);
    *p=(*p & ~(1 << bitpos)) | ((1&color) << bitpos);
  }
  /*
   * getpixel(x, y)
   *    x: horizontal pixel coordinate. Must be less than VGAX_WIDTH
   *    y: vertical pixel coordinate. Must be less than VGAX_HEIGHT
   *    return: 1bit color at <x,y> coordinate
   */
  static inline byte getpixel(byte x, byte y) {
    byte p=vgaxfb[y*VGAX_BWIDTH + (x>>3)], bitpos=(x & 7);
    return (p >> bitpos) & 1;
  }
  /*
   * putpixel8(B, y, color)
   *    bx: horizontal BYTE coordinate. Must be less than VGAX_BWIDTH
   *    y: vertical pixel coordinate. Must be less than VGAX_HEIGHT
   *    eightpixels: eight 1bit pixels in a byte.
   */
  static inline void putpixel8(byte bx, byte y, byte eightpixels) {
    vgaxfb[y*VGAX_BWIDTH + bx]=eightpixels;
  }
  /*
   * getpixel8(bx, y)
   *    bx: horizontal BYTE coordinate. Must be less than VGAX_BWIDTH
   *    y: vertical pixel coordinate. Must be less than VGAX_HEIGHT
   *    return: eight 1bit pixels packed in a byte
   */
  static inline byte getpixel8(byte bx, byte y) {
    return vgaxfb[y*VGAX_BWIDTH + bx];
  }
  /*
   * clear(color)
   *    color: 1bit color to clear the framebuffer
   */
  static void clear(byte color);
  /*
   * copy(src)
   *    src: source data. src size must be equal to framebuffer
   *
   * NOTE: pixels order is swapped (LSB-MSB) inside the same byte
   */
  static void copy(byte *src);
  /*
   * setExtendedColorsMask(mask)
   *    mask: 2 bits, enable additional extended colors generation. These
          2 bits will drive extended colors PINS: 6,7 for UNO, 30,31 for MEGA
   */
  static void setExtendedColorsMask(byte mask);
  /*
   * bitblit(src, swidth, sheight, dx, dy, color)
   *    src: source data. each byte hold 8 pixels. Bits set as 1 are opaque, 
   *      0 are transparent
   *    swidth: source width in pixels. This is the number of horizontal bits
   *      to be blitted and can be an integer not a multiple of 8
   *    sheight: source height in pixels
   *    dx: destination x coordinate in pixels. can be negative
   *    dy: destination y coordinate in pixels. can be negative
   *    color: 1bit color use for opaque pixels (with bits set to 1)
   */
  static void bitblit(byte *src, byte swidth, byte sheight, int dx, int dy,
                byte color);
  /*
   * blit(src, sx, sy, swidth, sheight, dx, dy)
   *    src: source data
   *    swidth: source width in pixels
   *    sheight: source height in pixels
   *    dx: destination x coordinate in pixels. can be negative
   *    dy: destination y coordinate in pixels. can be negative
   */
  static void blit(byte *src, byte swidth, byte sheight, int dx, int dy);
  /*
   * blitwmask(src, mask, sx, sy, swidth, sheight, dx, dy)
   *    src: source data. transparent pixels must be set to 0
   *    mask: source mask. pixel format is the same as src (1bit per pixel).
   *      value of 1 mean transparent pixel, value 0 mean opaque
   *    swidth: source width in pixels
   *    sheight: source height in pixels
   *    dx: destination x coordinate in pixels. can be negative
   *    dy: destination y coordinate in pixels. can be negative
   */
  static void blitwmask(byte *src, byte *mask, byte swidth, byte sheight,
                int dx, int dy);
  /*
   * print(fnt, glyphscount, fntheight, hspace, vspace, str, dx, dy, color)
   *    fnt: font definition, generated from 1BITFONT tool
   *    glyphscount: number of symbols inside font definition (generated from
   *      1BITFONT tool)
   *    fntheight: font height (generated from 1BITFONT tool)
   *    hspace: horizontal space between each printed symbol
   *    vspace: vertical space between each printed symbol
   *    str: string to be printed. The string is readed from PROGMEM (FLASH)
   *    dx: destination x coordinate in pixels. can be negative
   *    dy: destination y coordinate in pixels. can be negative
   *    color: color of the text
   */
  static void printPROGMEM(byte *fnt, byte glyphscount, byte fntheight, 
                byte hspace, byte vspace, const char *str, int dx, int dy, 
                byte color);
  /*
   * printSRAM(...)
   *    same as printPROGMEM but read str from SRAM
   *
   * NOTE: be carefull on Arduino UNO. SRAM has 128 bytes of free ram!
   */
  static void printSRAM(byte *fnt, byte glyphscount, byte fntheight, 
                byte hspace, byte vspace, const char *str, int dx, int dy, 
                byte color);
  /*
   * delay(msec)
   *    msec: number of milliseconds to wait
   */
  static void delay(int msec);
  /*
   * millis()
   *    return the number of milliseconds ellapsed
   */
  static inline unsigned millis() {
    return vtimer*16;
  }
  /*
   * micros()
   *    return the number of microseconds ellapsed  
   */
  static inline unsigned long micros() {
    return vtimer*16000;
  }
  /* 
   * rand()
   *    return random number
   */
  static unsigned rand();
};
#endif

