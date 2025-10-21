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

int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

int gQuitNow = 0;
int gFPS = 0;
int gTicks = 0;
int gLastFPS = 0;
int gFrameskip = 1;

RADPlayer gRadPlayer;
volatile int gTimerCounter = 0;

_go32_dpmi_seginfo gOldTimer, gNewTimer;
_go32_dpmi_seginfo gOldKB, gNewKB;

FILE *gLogFP = 0;


char gKeyDown[128];

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
  FILE *fp = fopen("data/tester.rad", "rb");
  char *data;
  long size = -1;

  if (!fp) {
    printf("Could not load audio\n");
    exit(3);
  }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  printf("Size is %d", (int)size);
  fseek(fp, 0, SEEK_SET);
  data = (char*)malloc(size);
  fread(data, 1, size, fp);
  fclose(fp);

  gRadPlayer.Init(data, NULL);
}

pcxFont_t gFont;

int main(int argc, char *argv[])
{
 int i;
 gameState_t gameState;
 //pcxImage_t extent;
 pcxImage_t fontImg, tex1;
 pcxImage_t dblBuf;
 atexit(cleanup);

gLogFP = fopen("log.txt", "w");

 setMode(0x13);

 startKeyboard();


 dblBuf.height = 200;
 dblBuf.width = 320;
 dblBuf.pixels = (unsigned char*)malloc(dblBuf.height*dblBuf.width);
 memset(dblBuf.pixels, 0, dblBuf.height*dblBuf.width);
 dblBuf.lines = (unsigned char**)malloc(sizeof(unsigned char*) * dblBuf.height);
 for (i=0; i < dblBuf.height; i++)
   dblBuf.lines[i] = dblBuf.pixels + dblBuf.width*i;


 // TODO: Don't set palette here-- this load is only to get the pal :)
 loadPCXFile(&tex1, "data/roadtex2.pcx");
 setPal(tex1.pal);
 
loadRAD();

 loadGfx();
 gameInit(&gameState);

 loadPCXFile(&fontImg, "data/font4x5.pcx");
 initFont(&fontImg, &gFont, 0, 4, 5);

 startTimer(70);

 // Start engine!
 triggerSfx(&gameState.sfxState, kSFX_Engine);
// gRadPlayer.SetSpeed(5);

 while (!gQuitNow)
 {

  if (!gFrameskip) {
    doGameLogic(&gameState); 
    doGameDraw(&gameState, &dblBuf);
  }
  while (gTimerCounter > 0) {
    if (gFrameskip) {
      doGameLogic(&gameState); 
    }
    if (++gTicks >= 70*2)
    {
      gLastFPS = gFPS / 2;
      gTicks = 0;
      gFPS = 0;
    }

    gTimerCounter--;
  }

  gFPS++;
  doGameDraw(&gameState, &dblBuf);
  waitVsync();
  flip(&dblBuf);
  doInput();
 
}

 
fclose(gLogFP);

 return 0;
}
