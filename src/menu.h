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

void initMenu(menuState_t *menu);
void doMenuLogic();
void doMenuDraw(pcxImage_t *dblBuf);

#endif
