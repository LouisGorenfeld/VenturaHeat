#include "draw.h"
#include "lmath.h"
#include "loadpcx.h"
#include "math.h" // abs

void hline(pcxImage_t *dest, int x1, int y, int x2, char color)
{
  if (x1 > x2)
  {
    int t = x1;
    x1 = x2;
    x2 = t;
  }

  if (y < 0 || y >= dest->height)
  {
    return;
  }


  if (x1 >= dest->width)
  {
    return;
  }
  else if (x1 < 0)
  {
    x1 = 0;
  }

  if (x2 >= dest->width)
  {
    x2 = dest->width - 1;
  }
  else if (x2 < 0)
  {
    x2 = 0;
  }

  while (x1 <= x2)
  {
    dest->lines[y][x1] = color;
    x1++;
  }
}

void vline(pcxImage_t *dest, int x, int y1, int y2, char color)
{

  if (y1 > y2)
  {
    int t = y1;
    y1 = y2;
    y2 = t;
  }

  if (x < 0 || x >= dest->width)
  {
    return;
  }


  if (y1 >= dest->height)
  {
    return;
  }
  else if (y1 < 0)
  {
    y1 = 0;
  }

  if (y2 >= dest->height)
  {
    y2 = dest->height - 1;
  }
  else if (y2 < 0)
  {
    y2 = 0;
  }

  while (y1 <= y2)
  {
    dest->lines[y1][x] = color;
    y1++;
  }
}

void drawSquare(pcxImage_t *dest, int x1, int y1, int x2, int y2, char color)
{
  int i;
  if (y1 > y2)
  {
    int t=y1;
    y1=y2;
    y2=t;
  }
  for (i=y1; i < y2; i++)
    hline(dest, x1, i, x2, color);
}

void drawLine(pcxImage_t *dest, int x1, int y1, int x2, int y2, char color)
{
  const int dx =  abs (x2 - x1);
  const int sx = (x1 < x2)? 1 : -1;
  const int dy = -abs (y2 - y1);
  const int sy = (y1 < y2)? 1 : -1;
  int err = dx + dy;
  int e2; 
 
  while (1) {
    *(dest->lines[y1] + x1) = color;
    if (x1 == x2 && y1 == y2) 
      break;
    
    e2 = err << 1;
    if (e2 >= dy) {
       err += dy; 
       x1 += sx; 
    } 
    if (e2 <= dx) { 
      err += dx; 
      y1 += sy; 
    }
  }
}

// TODO: Crop so we don't overwrite memory
void scaleBlit(pcxImage_t *img, pcxImage_t *dest,
  int x, int y, fixed_t dx, fixed_t dy)
{
  fixed_t i, j;
  unsigned char *r, *w, *wl;
  int sw = (img->width<<16) / dx;
  int sh = (img->height<<16) / dy;

  if (sw > dest->width)
    sw = dest->width;
  if (sh > dest->height)
    sh = dest->height;

  wl = dest->lines[y] + x;

  // TODO: What would i,j need to be to hit the clipping rectangle?
  // So, the question is (e.g.): at what point in the scaled graphic
  // is where does the scaled image hit the right edge of the screen, in
  // terms of its scaled 16.16 x coordinate?
  for (j=0; j < img->height << 16; j += dy)
  {
    w = wl;
    r = img->lines[j>>16];
    for (i=0; i < img->width << 16; i += dx)
      *w++ = *(r + (i>>16));

    wl += dest->width;
    y++;
  }
}



