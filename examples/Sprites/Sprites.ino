#include <VGAXUA.h>

VGAXUA vga;

//image generated from 1BITIMAGE - by Sandro Maffiodo
#define IMG_WIDTH 8
#define IMG_BWIDTH 1
#define IMG_HEIGHT 8
//data size=64 bytes
const unsigned char img_data[IMG_HEIGHT][IMG_BWIDTH] PROGMEM={
  { B00111100 },
  { B01000010 },
  { B10100101 },
  { B10011001 },
  { B01000010 },
  { B00111100 },
};
//image generated from 1BITIMAGE - by Sandro Maffiodo
#define MASK_WIDTH 8
#define MASK_BWIDTH 1
#define MASK_HEIGHT 8
//data size=64 bytes
const unsigned char mask_data[MASK_HEIGHT][MASK_BWIDTH] PROGMEM={
  { B00111100 },
  { B01111110 },
  { B11111111 },
  { B11111111 },
  { B01111110 },
  { B00111100 },
};
byte cnt=0;

void setup() {
  vga.begin();
  vga.clear(0);
}
void loop() {
  vga.blitwmask((byte*)img_data, (byte*)mask_data, IMG_WIDTH, IMG_HEIGHT, vga.rand()%(VGAX_WIDTH+10)-5, vga.rand()%(VGAX_HEIGHT+10)-5);
  if (cnt++>250) {
    vga.clear(0);
    cnt=0;
  }
  vga.setExtendedColorsMask(0);
}
