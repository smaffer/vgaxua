#include <VGAXUA.h>

VGAXUA vga;

void setup() {
  vga.begin();
  vga.clear(91);
  for (int y=0; y!=VGAX_HEIGHT; y++) {
    for (int x=0; x!=VGAX_WIDTH; x++) {
      if (x==0 || x==VGAX_WIDTH-1 || y==0 || y==VGAX_HEIGHT-1)
        vga.putpixel(x, y, 1);
      else
        vga.putpixel(x, y, vga.rand()%2);
    }
  }
}
void loop() {
  vga.setExtendedColorsMask(0);
}