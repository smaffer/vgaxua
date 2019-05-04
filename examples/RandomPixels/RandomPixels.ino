#include <VGAXUA.h>

VGAXUA vga;

void setup() {
  vga.begin();
}
void loop() {
  vga.putpixel(vga.rand()%VGAX_WIDTH, vga.rand()%VGAX_HEIGHT, vga.rand()%2);
  vga.setExtendedColorsMask(0);
}