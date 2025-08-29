#ifndef _DRAW_H_
#define _DRAW_H_

#include "loadpcx.h"

void hline(pcxImage_t *dest, int x1, int y, int x2, char color);
void vline(pcxImage_t *dest, int x1, int y, int x2, char color);
void drawSquare(pcxImage_t *dest, int x1, int y1, int x2, int y2, char color);

#endif

