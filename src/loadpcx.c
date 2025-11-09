#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loadpcx.h"


void loadPCX(pcxImage_t *img, unsigned char *data)
{
	unsigned char *p = data;
	unsigned char *w;
	int bytesPerPlaneLine;
	int bitsPerPixel;	// Bits per pixel per plane
    int pixelsPerByte;
	int numBitPlanes;
    int mask;
	int i,
	    j,
        k,
        l;
	int xMin,
	    yMin,
	    xMax,
	    yMax;
	int rl;

   	memset(img, 0, sizeof(pcxImage_t));
   
	if (*p++ != 0x0A)	{ // Manufacturer 
    printf("bad Manufacturer\n"); while(1);
		return;
  }
	p++;			// Skip version number
	if (*p++ != 0x01)	{ // Encoding
    printf("bad Encoding\n"); while(1);
		return;
  }

	bitsPerPixel = *p++;
    pixelsPerByte = 8 / bitsPerPixel;
    mask = (1 << bitsPerPixel) - 1;

	xMin = *p++;
	xMin += (*p++) << 8;

 	yMin = *p++;
	yMin += (*p++) << 8;
	xMax = *p++;
	xMax += (*p++) << 8;
	yMax = *p++;
	yMax += (*p++) << 8;

  	img->width = xMax - xMin + 1;
	img->height = yMax - yMin + 1;

	p += 4;		// Skip DPIs

	// Read 16-color palette. This will be overwritten if an 8bpp VGA palette
	// is specified at the end of the file.
	memcpy(img->pal, p, 48);
	p += 48;	

	p++;		// Skip reserved byte

    // We only support a single bitplane, even if it's 16 colors: No EGA.
	numBitPlanes = *p++;
    if (numBitPlanes != 1)
        return;
	
	bytesPerPlaneLine = *p++;
	bytesPerPlaneLine += (*p++) << 8;

//   // 0 == fucking deluxepaint 2
//	if (*p != 1 && *p != 64)	// Must be Color/BW palette and not greyscale
//		return;
   p++;
	// Skip rest of header
    
    
	p = data + 128;	// Seek to image data
	img->pixels = (unsigned char*)malloc(bytesPerPlaneLine * img->height * pixelsPerByte);
    img->lines = (unsigned char**)malloc(img->height * sizeof(unsigned char*));
	w = img->pixels;

	for (j=0; j < img->height; j++)
	{
		rl = 0;
		i = bytesPerPlaneLine;
		img->lines[j] = (unsigned char*)w;

//        printf("j=%d, relative address=%d\n", j, (int)(w - img->pixels));
        
		while (i > 0)
		{
			if (rl)
			{
                if (bitsPerPixel == 8)
                {
                    memset(w, *p, rl);
                    w += rl;
                }
                else
                {
//                    printf("Run of %d pixels, color %x\n", rl, (int)*p);
                    // Unpack the pixels into 8bpp space
                    for (l=0; l < rl; l++)
                    {
                        int shift = (pixelsPerByte - 1) * bitsPerPixel;
                        for (k=pixelsPerByte-1; k >= 0; k--)
                        {
                            *w++ = (*p >> shift) & mask;
                            shift -= bitsPerPixel;
                        }
                    }
                }
                
				i -= rl;
				rl = 0;
			}
			else
			{
				if (*p >= 192)
				{
					if (*p == 0xC0)		// Malformed file. Stop!
						return;

					rl = *p & 63;
				}
				else
				{
//                    printf("Run of 1 pixels, color %x\n", (int)*p);

                    if (bitsPerPixel == 8)
                    {
                        *w++ = *p;
                    }
                    else
                    {
                        int shift = (pixelsPerByte - 1) * bitsPerPixel;
                        for (k=pixelsPerByte-1; k >= 0; k--)
                        {
                            *w++ = (*p >> shift) & mask;
                            shift -= bitsPerPixel;
                        }
                    }
                    
					i--;
				}
			}

			p++;
		}
	}

	if (*p++ != 0x0C)
	{
		return;
	}

	memcpy(img->pal, p, 768);
}

void freePCX(pcxImage_t *img)
{
	free(img->pixels);
	free(img->lines);
}

void loadPCXFile(pcxImage_t *img, char *fn)
{
  FILE *fp;
	unsigned char *data;
	int len;

	fp = fopen(fn, "rb");
  if (!fp) {
    printf("Could not open %s", fn);
    while(1);
  }

	fseek(fp, 0, SEEK_END);
	len = (int)ftell(fp);
	
  data = (unsigned char*)malloc(len);
	fseek(fp, 0, SEEK_SET);
	fread(data, 1, len, fp);

	fclose(fp);	
	loadPCX(img, data);
}

void blitPCX(pcxImage_t *img, pcxImage_t *dest, int x, int y)
{
    blitPCXClip(img, dest, x, y, 0, 0, img->width, img->height);
}

void blitPCXClip(pcxImage_t *img, pcxImage_t *dest,
     int x, int y,
     int xStart, int yStart,
     int xEnd, int yEnd)
{

 int j;
 int width = xEnd - xStart;
 int height = yEnd - yStart;

 if (y+height < 0 || x+width < 0 || y >= 200 || x >= 320)
     return;
     
 if (y < 0)
 {
     yStart -= y;
     height += y;
     y = 0;
 }
 if (x < 0)
 {
     xStart -= x;
     width += xStart;
     x = 0;
 }
 if (y+height >= dest->height)
 {
     height -= y + height - dest->height;
     yEnd = dest->height - 1;
 }
 if (x+width >= 320)
 {
     width -= x + width - dest->width;
     xEnd = 319;
 }

 
 for (j=yStart; j < yEnd; j++)
 {
  unsigned char *wp = dest->lines[y] + x;
  unsigned char *rp = img->lines[j] + xStart;

  __asm__ __volatile__
  (
   "movl %0, %%ebx\n\t"
   "movl %1, %%ecx\n\t"
    "movl %2, %%edx\n\t"
  "loopPCX%=:\n\t"
    "movb (%%ecx), %%al\n\t"
    "movb %%al, (%%ebx)\n\t"
    "inc %%ebx\n\t"
    "inc %%ecx\n\t"
    "dec %%edx\n\t"
    "jnz loopPCX%="
    : // no outputs
    : "m"(wp), "m"(rp), "m"(width)
    : "%al", "%ebx", "%ecx", "%edx"
  );

  y++;
 }
}


void genSpriteMask(pcxImage_t *img, unsigned char *mask, int keyColor)
{
 int i, j;
 unsigned char *wp = mask;
 unsigned char *rp = img->pixels;

 for (j=0; j < img->height; j++)
 {
  for (i=0; i < img->width; i++)
  {
   if (*rp == keyColor)
      *wp = 0xFF;
   else
      *wp = 0x00;

   wp++;
   rp++;
  }
 }
}

void blitMaskSprite(pcxImage_t *img, unsigned char *mask, pcxImage_t *dest,
     int x, int y, int yClip)
{

 int j;
 y += yClip;

 for (j=yClip; j < img->height; j++,y++)
 {
  unsigned char *wp = dest->lines[y] + x;
  unsigned char *rp = img->lines[j];
  unsigned char *mp = mask + img->width*j;
  int width = (img->width >> 2) - 1;
  
    __asm__ __volatile__
  (
   "movl %0, %%ebx\n\t"
    "movl %1, %%ecx\n\t"
    "movl %2, %%edx\n\t"
    "movl %3, %%eax\n\t"

    ".align 4\n\t"
  "loopPCXMS%=:\n\t"
    "movl (%%eax,%%edx,4), %%esi\n\t"
    "andl %%esi, (%%ebx,%%edx,4)\n\t"
    "movl (%%ecx,%%edx,4), %%esi\n\t"
    "orl %%esi, (%%ebx,%%edx,4)\n\t"

    "decl %%edx\n\t"
    "jns loopPCXMS%=\n\t"
    : // no outputs
    : "m"(wp), "m"(rp), "m"(width), "m"(mp)
    : "%eax", "%ebx", "%ecx", "%edx", "%esi"
  );
 }
}

void blitPCXSprite(pcxImage_t *img, pcxImage_t *dest, int x, int y)
{

 int j;


 for (j=0; j < img->height; j++)
 {
  unsigned char *wp = dest->lines[y] + x;
  unsigned char *rp = img->lines[j];

    __asm__ __volatile__
  (
    "movl %0, %%ebx\n\t"
    "movl %1, %%ecx\n\t"
    "movl %2, %%edx\n\t"
  "loopPCX%=:\n\t"
    "movb (%%ecx,%%edx), %%al\n\t"
    "cmp $0, %%al\n\t"
    "je skipPixelPCX%=\n\t"
    "movb %%al, (%%ebx,%%edx)\n\t"
  "skipPixelPCX%=:\n\t"
    "dec %%edx\n\t"
    "jnz loopPCX%="
    : // no outputs
    : "m"(wp), "m"(rp), "m"(img->width)
    : "%al", "%ah", "%ebx", "%ecx", "%edx"
  );
  
  y++;
 }
}

void blitPCXScaledSprite(pcxImage_t *img, pcxImage_t *dest,
     int x, int y,
     fixed_t xScale, fixed_t yScale,
     int yClip)
{
  if (xScale == 0 || yScale == 0)
    return;

  fixed_t rx, ry;
  fixed_t dx = (1<<FIXSHIFT<<8) / (xScale >> 8);
  fixed_t dy = (1<<FIXSHIFT<<8) / (yScale >> 8);
  fixed_t rxBegin = 0;
  int xClip;

  rx = 0;
  ry = 0;

  if (x >= dest->width)
    return;

  if (y >= dest->height)
    return;



  xClip = x + FIXTOI(xScale * img->width);
  if (x < 0)
  {
    rxBegin = dx * -x;
    x = 0;
  }


  // TODO: allow the passed-in yClip to override this
  yClip = y + FIXTOI(yScale * img->height);
  if (y < 0)
  {
    ry = dy * -y;
    y = 0;
  }
  
  if (xClip > dest->width)
    xClip = dest->width;

  if (yClip > dest->height)
    yClip = dest->height;

  for (; y < yClip; y++, ry += dy)
  {
    unsigned char *wline = dest->lines[y];
    unsigned char *rline = img->lines[FIXTOI(ry)];
    int i;

    rx = rxBegin;
    for (i=x; i < xClip; i++, rx += dx)
    {
      unsigned char pixel = rline[FIXTOI(rx)];
      if (pixel)
        wline[i] = pixel;
    }
  }
}

void transposePCX(pcxImage_t *img, int index)
{
 int i;
 for (i=0; i < img->width*img->height; i++)
     img->pixels[i] += index;
}



