#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dpmi.h>
#include <pc.h>
#include <go32.h>
#include <sys/farptr.h>
#include <crt0.h>
#include <malloc.h>
#include "player20.h"
#include "loadpcx.h"
#include "pcxfont.h"
#include "road.h"
#include "game.h"
#include "draw.h"
#include "data.h"
#include "records.h"
#include "menu.h"

typedef void (*logicFn_t)(void*);
typedef void (*drawFn_t)(void*, pcxImage_t*);

typedef struct screen_t
{
  logicFn_t logicFn;
  drawFn_t drawFn;
  void *state;  // screen-specific state
} screen_t;

screen_t gScreenMenu;
screen_t gScreenGame;
screen_t gCurScreen;

int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

int gQuitNow = 0;
int gFPS = 0;
int gTicks = 0;
int gLastFPS = 0;
int gFrameskip = 1;

unsigned char *gTitlePal;
unsigned char *gGamePal;

RADPlayer gRadPlayer;
volatile int gTimerCounter = 0;

_go32_dpmi_seginfo gOldTimer, gNewTimer;
_go32_dpmi_seginfo gOldKB, gNewKB;

FILE *gLogFP = 0;

pcxImage_t gTitleImg, gFontDigiImg, gTachImg;
pcxFont_t gFontTiny, gFontMedium, gFontDefault;

char gKeyDown[128];

void clearImage(pcxImage_t*);
void fadeOutPal(unsigned char pal[]);
void fadeInPal(unsigned char pal[]);
void blankPal();

void handleTimer()
{
  gRadPlayer.Update();
  gTimerCounter++;
}

int gTimerInit=0;
void startTimer(int speed)
{
	outportb(0x43, 0x34);
  const int pitSpeed = 1193180;
  speed = pitSpeed/speed;
	outportb(0x40, (speed & 0xFF));
	outportb(0x40, (speed >> 8) & 0xFF);

	_go32_dpmi_get_protected_mode_interrupt_vector(8, &gOldTimer);
	gNewTimer.pm_offset = (int)handleTimer;
	gNewTimer.pm_selector = _go32_my_cs();
	_go32_dpmi_chain_protected_mode_interrupt_vector(8, &gNewTimer);

	gTimerInit=1;
}

void stopTimer()
{
 if (gTimerInit)
 {
    _go32_dpmi_set_protected_mode_interrupt_vector(8, &gOldTimer);

    outportb(0x43, 0x34);
    outportb(0x40, 0);
    outportb(0x40, 0);

 }
 
 gTimerInit=0;
}




void handleKeyboard()
{
  int scan = (int)inportb(0x60);
  char status = inportb(0x61);

  if (scan & 0x80)
    gKeyDown[scan & 0x7f] = 0;
  else
    gKeyDown[scan] = 1;
  
  outportb(0x61, status | 0x80);
  outportb(0x61, status);
  
  outportb(0x20, 0x20);   // reset PIC
  
}

void startKeyboard()
{
	_go32_dpmi_get_protected_mode_interrupt_vector(9, &gOldKB);
	gNewKB.pm_offset = (int)handleKeyboard;
	gNewKB.pm_selector = _go32_my_cs();
	_go32_dpmi_chain_protected_mode_interrupt_vector(9, &gNewKB);

   memset(gKeyDown, 0, 128);
}

void stopKeyboard()
{
    _go32_dpmi_set_protected_mode_interrupt_vector(9, &gOldKB);
}




void flip(pcxImage_t *buf)
{
  movedata(
           _my_ds(),
           (unsigned)buf->pixels,
           _dos_ds,
           0xA0000,
           buf->height*buf->width
  );
}

void setMode(int mode)
{
 __dpmi_regs regs;
 memset(&regs, 0, sizeof(regs));
 regs.x.ax = mode;
 __dpmi_int(0x10, &regs);
}

// Checking input is for keyboard buffering. We should
// not poll but handle int 9.
void doInput()
{
 char ch;
  __dpmi_regs regs;
  memset(&regs, 0, sizeof(regs));
  regs.h.ah = 0x1;
  __dpmi_int(0x16, &regs);
  ch = regs.h.ah;

  if ((regs.x.flags & 0x0040) == 0)
  {
    memset(&regs, 0, sizeof(regs));
    __dpmi_int(0x16, &regs);

    if (ch == 0x01)  // esc
    {
       gQuitNow = 1;
    }
  }
  
}

// There might be an interrupt we can latch onto
// instead, though this does work
void checkPlayer();
void waitVsync()
{
 while (inportb(0x3DA) & 8);
 while (!(inportb(0x3DA) & 8));
}

void cleanup()
{
 setMode(0x2);
 stopTimer();
 stopKeyboard();
}

void setPal(unsigned char *pal)
{
 int j=0;
 int i;
 for (i=0; i < 768; i+=3, j++)
 {
  outportb(0x3C8, j);
  outportb(0x3C9, pal[i]>>2);
  outportb(0x3C9, pal[i+1]>>2);
  outportb(0x3C9, pal[i+2]>>2);
 }
}

void drawSpriteSlow(pcxImage_t *src, unsigned char *dest, int x, int y)
{
 unsigned char *w;
 unsigned char *r;
 unsigned char *end;
 int j;
 int line=y;

 for (j=0; j < src->height; j++, line++)
 {
  w = dest + 320*line + x;
  r = src->lines[j];

  end = r + src->width;
  while (r != end)
  {
   if (*r)
      *w = *r;
   w++;
   r++;
  }
 }

}


void setColor(unsigned char *pal, int index, char r, char g, char b)
{
 pal += index * 3;
 *pal++ = r;
 *pal++ = g;
 *pal++ = b;
}

int sbPort()
{
  char *b = getenv("BLASTER");
  char *c = b;

  if (!b)
     return 0x388;

  while (*b != '\0' && *b != 'A' && *b != 'a')
      b++;  
  while (*b < '0' && *b > '9')
      b++;
  b++;


  c = b;
  while (*b != '\0' && *b != ' ' && (*b >= '0' && *b <= '9'))
        b++;

  *b = '\0';
  return strtol(c, NULL, 16);
}

void loadRAD() {
  gRadPlayer.Init(data_main, NULL);
}


void blankPal() {
  unsigned char pal[768];
  memset(pal, 0, 768);
  setPal(pal);    
}

void menuToGame(pcxImage_t *dblBuf)
{
  fadeOutPal(gTitlePal);
  clearImage(dblBuf);
  flip(dblBuf);
  setPal(gGamePal);

  // TODO
}


void fadeInPal(unsigned char pal[]) 
{
  for (int j=256; j >= 0; j--) {
    unsigned char p[768];
    for (int i=0; i < 768; i++) {
      int x = pal[i] - j;
      if (x < 0)
        x = 0;
      p[i] = x;
    }
    setPal(p);
    waitVsync();
  }
}

void fadeOutPal(unsigned char pal[]) 
{
  for (int j=0; j < 256; j+=4) {
    unsigned char p[768];
    for (int i=0; i < 768; i++) {
      int x = pal[i] - j;
      if (x < 0)
        x = 0;
      p[i] = x;
    }
    setPal(p);
    waitVsync();
  }
}

void fadePalToWhite(unsigned char pal[]) 
{
  for (int j=0; j < 256; j+=4) {
    unsigned char p[768];
    for (int i=0; i < 768; i++) {
      int x = pal[i] + j;
      if (x > 255)
        x = 255;
      p[i] = x;
    }
    setPal(p);
    waitVsync();
  }
}

void showRaceOver(pcxImage_t *dblBuf, gameState_t *state)
{
  fadePalToWhite(gGamePal);
  unsigned char p[768];
  memset(p, 255, 768);
  clearImage(dblBuf);
  
  p[15*3] = 0;
  p[15*3 + 1] = 0;
  p[15*3 + 2] = 0;
  setPal(p);

  int y=50;
  char *thanks = "THANK YOU FOR PLAYING";
  blitFont(&gFontMedium, dblBuf->pixels, thanks, (320-(gFontMedium.fontWidth*strlen(thanks)))>>1, y);

  y += gFontMedium.fontHeight * 2;
  char results[128];

  flip(dblBuf);
  for (int i=0; i < 2*70; i++) waitVsync();

  int dnf = 0;
  for (int i=0; i < MAX_STAGES; i++) {
    int time = state->laptimes[i];
    float fTime = (float)time / 70.0f;
    if (fTime > 0) {
      sprintf(results, "LAP %d - %.02f SEC", i+1, fTime); 
    }
    else {
      sprintf(results, "LAP %d - DNF", i+1);
      dnf = 1;
    }
    blitFont(&gFontTiny, dblBuf->pixels, results, (320-(gFontTiny.fontWidth*strlen(results)))>>1, y);
    y += gFontTiny.fontHeight + 2;
    flip(dblBuf);
    for (int j=0; j < 45; j++) waitVsync();
  }

  for (int j=0; j < 70; j++) waitVsync();
  y += 5;
  if (dnf == 0)
    sprintf(results, "TOTAL TIME: %.02f SEC", (float)state->totaltime / 70.0f);
  else
    sprintf(results, "PLEASE TRY AGAIN");
  blitFont(&gFontTiny, dblBuf->pixels, results, (320-(gFontTiny.fontWidth*strlen(results)))>>1, y);

  flip(dblBuf);
  for (int i=0; i < 10*70; i++)
    waitVsync();

  fadeOutPal(p);
}

void showTitle(pcxImage_t *dblBuf)
{
  clearImage(dblBuf);
//  setPal(gTitlePal);
  blankPal();
  blitPCX(&gTitleImg, dblBuf, 160-(gTitleImg.width>>1), 0);  
  flip(dblBuf);
  fadeInPal(gTitlePal);
  blitFont(&gFontTiny, dblBuf->pixels, "DOSTEMBER GAMEJAM EDITION 1.1, NOVEMBER 2025 BY EXTENT OF THE JAM", 0, 0);
  blitFont(&gFontMedium, dblBuf->pixels, "PRESS ACCELERATOR TO START", 0, 170);
  blitFont(&gFontMedium, dblBuf->pixels, "ESC TO QUIT TO DOS", 0, 180);
}

void clearImage(pcxImage_t *p)
{
  memset(p->pixels, 0, p->height*p->width);
}

void initScreens() 
{
}

int main(int argc, char *argv[])
{
 int i;
 gameState_t gameState;
 menuState_t menuState;
 pcxImage_t fontImg, fontMediumImg, fontDefaultImg, tex1;
 pcxImage_t dblBuf;

 atexit(cleanup);

//gLogFP = fopen("log.txt", "w");

 setMode(0x13);

 startKeyboard();

 dblBuf.height = 200;
 dblBuf.width = 320;
 dblBuf.pixels = (unsigned char*)malloc(dblBuf.height*dblBuf.width);
 dblBuf.lines = (unsigned char**)malloc(sizeof(unsigned char*) * dblBuf.height);
 for (i=0; i < dblBuf.height; i++)
   dblBuf.lines[i] = dblBuf.pixels + dblBuf.width*i;


 gScreenGame.drawFn = doGameDraw;
 gScreenGame.logicFn = doGameLogic;
 gScreenGame.state = (void*)&gameState;

 gScreenMenu.drawFn = doMenuDraw;
 gScreenMenu.logicFn = doMenuLogic;
 gScreenMenu.state = (void*)&menuState;

 loadRAD();

 loadGfx();
 gameInit(&gameState);
 menuInit(&menuState);
 loadRecords(&gameState);

 loadPCX(&fontImg, data_font4x5);
 initFont(&fontImg, &gFontTiny, 0, 4, 5);

 loadPCX(&fontMediumImg, data_f10x10);
 initFont(&fontMediumImg, &gFontMedium, 0, 10, 10);

 loadPCX(&fontDefaultImg, data_f6x6);
 initFont(&fontDefaultImg, &gFontDefault, 0, 8, 8);

 loadPCX(&gFontDigiImg, data_diginums);

 loadPCX(&gTachImg, data_digitach);

 loadPCX(&gTitleImg, data_vhlogo);

 loadPCX(&tex1, data_roadtex2);   // harvest palette
 gGamePal = tex1.pal;
 gTitlePal = gTitleImg.pal;

 startTimer(70);

 // Start engine!
 triggerSfx(&gameState.sfxState, kSFX_Engine);
 gRadPlayer.SetSpeed(5);


// showTitle(&dblBuf);

gCurScreen = gScreenMenu;

 while (!gQuitNow && !gameState.crossedFinish && gameState.gameOverCountdown != 0)
 {

  if (!gFrameskip) {
    gCurScreen.logicFn(&gameState); 
    gCurScreen.drawFn(&gameState, &dblBuf);
    gFPS++;
  }
  else {
    while (gTimerCounter > 0) {
      if (gFrameskip) {
        gCurScreen.logicFn(&gameState);
      }
      
      gTimerCounter--;
    }

    gFPS++;
  }

  // Handle FPS logic
  if (++gTicks >= 70*2)
  {
    gLastFPS = gFPS / 2;
    gTicks = 0;
    gFPS = 0;
  }

  // Draw FPS before flipping
//    char dbgline[128];
//    sprintf(dbgline, "FPS: %d", gLastFPS);
//    blitFont(&gFontTiny, dblBuf.pixels, dbgline, 75, 8);

  gCurScreen.drawFn(&gameState, &dblBuf);
  waitVsync();
  flip(&dblBuf);
  doInput();
}

refreshRecords(&gameState);
saveRecords(&gameState);
killSfx(&gameState.sfxState);
if (!gQuitNow) {
  showRaceOver(&dblBuf, &gameState);
}
else {  // ESC
  fadeOutPal(gGamePal);
}



cleanup();
gRadPlayer.Stop();

 return 0;
}
