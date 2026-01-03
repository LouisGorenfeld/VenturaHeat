#include <stdio.h>
#include <string.h>
#include "main.h"
#include "menu.h"
#include "config.h"

typedef enum mainMenu_t
{
    MENU_START,
    MENU_REDEFINE,
    MENU_RECORDS,
    MENU_README,
    MENU_QUIT,
    MENU_MAX
}  mainMenu_t;

char inKey() 
{
  while (!gLastKeyDown);
  return gLastKeyDown;
}

void menuInit(menuState_t *state)
{
  memset(state, 0, sizeof(menuState_t));
}

void showRecords(menuState_t *state)
{
  // TODO
}

void doReassignKeys(menuState_t *state)
{
  config_t *config;
  config->keyLeft = inKey();
  config->keyRight = inKey();
  config->keyAccel = inKey();
  config->keyBrake = inKey();
  config->keyShiftUp = inKey();
  config->keyShiftDn = inKey();
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
      if (state->curIdx < MENU_MAX)
        state->curIdx++;
    }
    state->keyNextItem = 1;
  }
  else
  {
    state->keyNextItem = 0;
  }

  if (gKeyDown[0x1C]) { // enter
    switch (state->curIdx) {
      case MENU_START:
        menuToGame();
        break;
      case MENU_REDEFINE:
        doReassignKeys(state);
        break;
      case MENU_RECORDS:
        showRecords(state);
        break;
    }
  }
}

void doMenuDraw(void *s, pcxImage_t *dblBuf)
{
  static const char* const menu[] = {"START", "REDEFINE KEYS", "RECORDS", "DOCS", "EXIT TO DOS"};
  static const int kXMenu = 100;
  static const int kYMenu = 120;
  menuState_t *state = (menuState_t*)s;

  clearImage(dblBuf);
  blitPCX(&gTitleImg, dblBuf, 160-(gTitleImg.width>>1), 0);  

  for (int i=0; i < MENU_MAX; i++) 
  {
    blitFont(&gFontDefault, dblBuf->pixels, menu[i], kXMenu, kYMenu + i*10);
  }

  blitFont(&gFontDefault, dblBuf->pixels, "(", 
    kXMenu - (gFontDefault.fontWidth<<1), 
    kYMenu + state->curIdx*gFontDefault.fontHeight);
}

