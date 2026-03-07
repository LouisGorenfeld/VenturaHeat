#ifndef MENU_H
#define MENU_H

#include "loadpcx.h"

typedef struct config_t config_t;  // Forward declaration

typedef struct menuState_t
{
    int state;
    int subState;

    int curIdx;

    // Booleans for picking up keypresses
    int keyNextItem;
    int keyPrevItem;
    int keySelectItem;
    int keyBackOut;
    
    config_t *config;  // Pointer to configuration
} menuState_t;

void menuInit(menuState_t *state, config_t *config);
void doMenuLogic(void *state);
void doMenuDraw(void *state, pcxImage_t *dblBuf);

#endif
