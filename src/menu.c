#include "menu.h"

extern char gKeyDown[128];


typedef enum mainMenu_t
{
    MENU_START,
    MENU_REDEFINE,
    MENU_RECORDS,
    MENU_README,
    MENU_QUIT,
    MENU_MAX
}  mainMenu_t;


void doMenuLogic(menuState_t *state)
{

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

void doMenuDraw(pcxImage_t *dblBuf);
{
  if (gKeyDown[0x48]) 
    menuToGame(dblBuf);
}
