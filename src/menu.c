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

void menuInit(menuState_t *state, config_t *config)
{
  memset(state, 0, sizeof(menuState_t));
  state->config = config;
}

void showRecords(menuState_t *state)
{
  // TODO
}

static void showKeyPrompt(const char *prompt)
{
  if (!gDblBuf) return;
  
  int textWidth = strlen(prompt) * gFontDefault.fontWidth;
  int x = (320 - textWidth) >> 1; 
  int y = 100; 
  
  clearImage(gDblBuf);
  blitPCX(&gTitleImg, gDblBuf, 160-(gTitleImg.width>>1), 0);
  blitFont(&gFontDefault, gDblBuf->pixels, (char*)prompt, x, y);
  flip(gDblBuf);
}

void doReassignKeys(menuState_t *state)
{
  if (!state || !state->config) return;  // Null check
  
  config_t *config = state->config;
  
  waitForAllKeysReleased();
  
  showKeyPrompt("PRESS KEY TO STEER LEFT");
  config->keyLeft = inKey();
  
  showKeyPrompt("PRESS KEY TO STEER RIGHT");
  config->keyRight = inKey();
  
  showKeyPrompt("PRESS KEY FOR ACCELERATOR");
  config->keyAccel = inKey();
  
  showKeyPrompt("PRESS KEY FOR BRAKE");
  config->keyBrake = inKey();
  
  showKeyPrompt("PRESS KEY TO SHIFT UP");
  config->keyShiftUp = inKey();
  
  showKeyPrompt("PRESS KEY TO SHIFT DOWN");
  config->keyShiftDn = inKey();
  
  showKeyPrompt("KEYS CONFIGURED!");
  
  writeConfig(config);
  
  for (int i = 0; i < 70; i++) {  // wait 1 second
    waitVsync();
  }
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
  static const int kYPad = 10;
  menuState_t *state = (menuState_t*)s;

  clearImage(dblBuf);
  blitPCX(&gTitleImg, dblBuf, 160-(gTitleImg.width>>1), 0);  

  for (int i=0; i < MENU_MAX; i++) 
  {
    blitFont(&gFontDefault, dblBuf->pixels, menu[i], kXMenu, kYMenu + i*kYPad);
  }

  blitFont(&gFontDefault, dblBuf->pixels, "(", 
    kXMenu - (gFontDefault.fontWidth<<1), 
    kYMenu + state->curIdx*kYPad + 1);
}

