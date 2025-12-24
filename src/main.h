#ifndef MAIN_H
#define MAIN_H

#include "loadpcx.h"
#include "pcxfont.h"
#include "game.h"

// Palette functions
void fadeOutPal(unsigned char pal[]);
void fadeInPal(unsigned char pal[]);
void setPal(unsigned char *pal);
void blankPal();
void setColor(unsigned char *pal, int index, char r, char g, char b);

// Image functions
void clearImage(pcxImage_t*);
void flip(pcxImage_t *buf);
void waitVsync();

// Transitions
void menuToGame(pcxImage_t *dblBuf);
void showRaceOver(pcxImage_t *dblBuf, gameState_t *state);
void showTitle(pcxImage_t *dblBuf);

// Crap that should be in gfxstore
extern pcxImage_t gTitleImg, gFontDigiImg, gTachImg;
extern pcxFont_t gFontTiny, gFontMedium, gFontDefault;

#endif
