#ifndef MENU_H
#define MENU_H

#include "loadpcx.h"

typedef struct menuState_t
{
    int curIdx;

    int keyNextItem;
    int keyPrevItem;
    int keySelectItem;
    int keyBackOut;
} menuState_t;

void menuInit(menuState_t *state);
void doMenuLogic(void *state);
void doMenuDraw(void *state, pcxImage_t *dblBuf);

#endif
