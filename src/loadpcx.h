#ifndef _LOADPCX_H_
#define _LOADPCX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lmath.h"

typedef struct pcxImage_t pcxImage_t;
struct pcxImage_t
{
 unsigned char pal[768];
 unsigned char *pixels;
 unsigned char **lines;
 int width;
 int height;
};

void loadPCX(pcxImage_t *img, unsigned char *data);

void loadPCXFile(pcxImage_t *img, char *fn);

void freePCX(pcxImage_t *img);

void blitPCX(pcxImage_t *img, pcxImage_t *dest,
     int x, int y);

// TODO: Wouldn't this be better if it clipped against the destination?
// (e.g., clip against the horizon; see blitPCXScaledSprite)
void blitPCXClip(pcxImage_t *img, pcxImage_t *dest,
     int x, int y,
     int xStart, int yStart,    // Source's upper left corner
     int xEnd, int yEnd);       // Source's lower right corner

void blitPCXSprite(pcxImage_t *img, pcxImage_t *dest,
     int x, int y);

void blitPCXScaledSprite(pcxImage_t *img, pcxImage_t *dest,
     int x, int y,
     fixed_t xScale, fixed_t yScale,    // dx and dy are 1/scale, also known as z...
     int yClip);                // Stop drawing if below destination yClip
     

void genSpriteMask(pcxImage_t *img, unsigned char *mask, int keyColor);

void blitMaskSprite(pcxImage_t *img, unsigned char *mask,
     pcxImage_t *dest,
     int x, int y,
     int yClip);

void transposePCX(pcxImage_t *img, int index);

#ifdef __cplusplus
}
#endif

#endif

