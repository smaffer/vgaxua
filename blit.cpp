#include "VGAXUA.h"

void VGAXUA::blit(byte *src, byte swidth, byte sheight, int dx, int dy) {
  byte sh=sheight;
  byte *srcline=src;
  byte slinesize=swidth>>3;
  if (swidth & 7)
      slinesize++;
  if (dx>-1 && dx+swidth<=VGAX_WIDTH && dy>-1 && dy+sheight<=VGAX_HEIGHT) {
    //inside screen. no clipping
    while (sh--) {
      byte sw=swidth, sx=0;
      int ldx=dx;
      while (sw--) {
        //get sprite pixel
        byte p=pgm_read_byte(srcline + (sx>>3));
        byte bitpos=(sx & 7);
        byte spixel=(p >> bitpos) & 1;
        //set framebuffer pixel
        putpixel(ldx, dy, spixel);
        sx++;
        ldx++;
      }
      srcline+=slinesize;
      dy++;
    }
  } else {
    //partially clipped out of screen
    while (sh--) {
      if (dy>-1 && dy<VGAX_HEIGHT) {
        byte sw=swidth, sx=0;
        int ldx=dx;
        while (sw--) {
          if (ldx>-1 && ldx<VGAX_WIDTH) {
            //get sprite pixel
            byte p=pgm_read_byte(srcline + (sx>>3));
            byte bitpos=(sx & 7);
            byte spixel=(p >> bitpos) & 1;
            //set framebuffer pixel
            putpixel(ldx, dy, spixel);
          }
          sx++;
          ldx++;
        }
      }
      srcline+=slinesize;
      dy++;
    }
  }
}
void VGAXUA::blitwmask(byte *src, byte *mask, byte swidth, byte sheight, int dx, int dy) {
  byte sh=sheight;
  byte *srcline=src;
  byte *maskline=mask;
  byte linesize=swidth>>3;
  if (swidth & 7)
      linesize++;
  while (sh--) {
    if (dy>-1 && dy<VGAX_HEIGHT) {
      byte sw=swidth, sx=0;
      int ldx=dx;
      while (sw--) {
        if (ldx>-1 && ldx<VGAX_WIDTH) {
          //get mask bit
          byte m=pgm_read_byte(maskline + (sx>>3));
          byte p=pgm_read_byte(srcline  + (sx>>3));
          //get sprite pixel and mask
          byte sbitpos=(sx & 7);
          byte spixel=(p >> sbitpos) & 1;
          byte mpixel=(m >> sbitpos) & 1;
          //set framebuffer pixel with AND+OR blit
          if (mpixel)
            putpixel(ldx, dy, spixel);
          /*
          byte *pfb=vgaxfb + dy*VGAX_BWIDTH + (ldx>>3);
          byte dbitpos=7-(ldx & 7);
          *pfb &=(mpixel << dbitpos) | ~(1<<dbitpos);
          *pfb |=(spixel << dbitpos);
          */
        }
        sx++;
        ldx++;
      }
    }
    srcline +=linesize;
    maskline+=linesize;
    dy++;
  }
}
