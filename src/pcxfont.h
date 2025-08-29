#ifndef _PCXFONT_H_
#define _PCXFONT_H_


#include "loadpcx.h"

#ifdef __cplusplus
extern "C" {
#endif

// Each field must be filled out
typedef struct pcxFont_t pcxFont_t;
struct pcxFont_t
{
 // Padded version of the font (so char widths are divisible by 4 pixels)
 unsigned int *chrImage;
 unsigned int *maskImage;
 int fontWidth;
 int fontHeight;
 int longsPerLinePerChar;   // 32-bit fetches per row per character
};

void blitFont(pcxFont_t *font, unsigned char *destBitmap,
     char *string, int x, int y);

void uninitFont(pcxFont_t *font);

// Turn this font into a dither font (was makeDitherFont)
void initFont(pcxImage_t *img, pcxFont_t *font,
     int keyColor, int w, int h);

#ifdef __cplusplus
}
#endif

#endif

