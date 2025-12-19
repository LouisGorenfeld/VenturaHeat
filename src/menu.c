#include <stdio.h>
#include <string.h>
#include "menu.h"
#include "pcxfont.h"

extern char gKeyDown[128];
extern pcxImage_t gTitleImg;
extern pcxFont_t gFontDefault;

typedef enum mainMenu_t
{
    MENU_START,
    MENU_REDEFINE,
    MENU_RECORDS,
    MENU_README,
    MENU_QUIT,
    MENU_MAX
}  mainMenu_t;

void menuInit(menuState_t *state)
{
  memset(state, 0, sizeof(menuState_t));
}

void doMenuLogic(void *s)
{
  menuState_t *state = s;

  if (gKeyDown[0x48]) // up 
  {
    if (!state->keyPrevItem) 
    {
      if (state->curIdx > 0)
        state->curIdx--;
    }
    state->keyPrevItem = 1;
  }
  else 
  {
    state->keyPrevItem = 0;
  }

  if (gKeyDown[0x50]) // down
  {
    if (!state->keyNextItem) 
    {
      if (state->curIdx < MENU_MAX-1)
        state->curIdx++;
    }
    state->keyNextItem = 1;
  }
  else
  {
    state->keyNextItem = 0;
  }
}

void doMenuDraw(void *s, pcxImage_t *dblBuf)
{
  menuState_t *state = (menuState_t*)s;
  static const char* menu[] = {"START", "REDEFINE KEYS", "RECORDS", "DOCS", "EXIT TO DOS"};
  static const int kXMenu = 50;
  static const int kYMenu = 50;



  for (int i=0; i < MENU_MAX; i++) 
  {
    blitFont(&gFontDefault, dblBuf->pixels, menu[i], kXMenu, kYMenu + i*10);
  }

  blitFont(&gFontDefault, dblBuf->pixels, "O", 
    kXMenu - gFontDefault.fontWidth, 
    kYMenu + state->curIdx*gFontDefault.fontHeight);
}
