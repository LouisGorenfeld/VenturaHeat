#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pcxfont.h"

void blitChar(pcxFont_t *font, char c, int x, int y,
     unsigned char *dest)
{
  int i, j;
  int rx, ry;
  int incr = font->longsPerLinePerChar;
  int readLineIncr;
  unsigned int *wp;
  unsigned int *rp;
  unsigned int *mp;

  // All alpha chars are same case, line 1
  // Numbers, line 2
  // Punctuation after numbers in ASCII order, line 2
  if (c >= 'a' && c <= 'z')
  {
    ry = 0;
    rx = (c - 'a') * incr;
  }
  else if (c >= 'A' && c <= 'Z')
  {
    ry = 0;
    rx = (c - 'A') * incr;
  }
  else if (c >= '0' && c <= '9')
  {
    ry = font->fontHeight;
    rx = (c - '0') * incr;
  }
  else if (c >= 33 && c <= 47)
  {
    ry = font->fontHeight;
    rx = (c - 33 + 10) * incr;
  }
  else
  {
     // Skip it
     return;
  }

  readLineIncr = font->longsPerLinePerChar*26;

  wp = (unsigned int*)&dest[320*y + x];
  rp = font->chrImage + font->longsPerLinePerChar*(ry*26) + rx;
  mp = font->maskImage + font->longsPerLinePerChar*(ry*26) + rx;

  for (j=0; j < font->fontHeight; j++)
  {
    for (i=0; i < font->longsPerLinePerChar; i++)
    {
      wp[i] &= mp[i];
      wp[i] |= rp[i];
    }

    rp += readLineIncr;
    mp += readLineIncr;
    wp += 80;
  }
}



void uninitFont(pcxFont_t *font)
{
 free(font->chrImage);
 free(font->maskImage);
}

// Turn this pcx into a masked font
// It is padded to a multiple of 4 bytes
void initFont(pcxImage_t *img, pcxFont_t *f,
     int keyColor, int w, int h)
{
 int i, j, k;
 int imgSize;
 unsigned char *rp;
 unsigned char *wp;
 unsigned char *mp;

 f->fontWidth = w;
 f->fontHeight = h;

 if (w%4 != 0)
    f->longsPerLinePerChar = (w>>2) + 1;
 else
    f->longsPerLinePerChar = w>>2;

 // 2 rows, 26 chars
 imgSize = f->longsPerLinePerChar * 26 * f->fontHeight * 2;
 f->chrImage = (unsigned int*)malloc(imgSize*sizeof(int));
 f->maskImage = (unsigned int*)malloc(imgSize*sizeof(int));

 memset(f->chrImage, 0x00, imgSize*sizeof(int));
 memset(f->maskImage, 0xFF, imgSize*sizeof(int));

 for (k=0; k < 26; k++) // for each char
 {
   for (j=0; j < h*2; j++) // height of both char rows
   {
     rp = img->lines[j] + w*k;
     wp = (unsigned char*)&f->chrImage[(26*j + k) * f->longsPerLinePerChar];
     mp = (unsigned char*)&f->maskImage[(26*j + k) * f->longsPerLinePerChar];

     for (i=0; i < w; i++)
     {
       if (*rp == keyColor)
       {
         *wp = 0x00;
         *mp = 0xFF;
       }
       else
       {
         *wp = *rp;
         *mp = 0x00;
       }

       wp++;
       rp++;
       mp++;
     }
   }
 }
}


void blitFont(pcxFont_t *font, unsigned char *dest, char *string, int x, int y)
{
  char *s = string;

 while (*s != '\0')
 {
  blitChar(font, *s, x, y, dest);

  x += font->fontWidth;
  s++;
 }
}

