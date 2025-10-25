#include "cars.h"
#include "roadctlr.h"
#include "road.h"
#include "loadpcx.h"
#include "sfx.h"

// This module is a lot of the overarching game logic

#ifndef _GAME_H_
#define _GAME_H_



// Convert speed in MPH fixed point to Z for scroll speeds (>>3 is arbitrary)
#define SPEEDTOZ(a) ((a)>>16>>3)

// Cars house the logic for the road objects as well as providing
// allocation for the display list object, which they manipulate

// This uses ints for easy clamping
typedef struct plrCtrls_t
{
  int wheel;   // -127 - 127
  int gas;     // 0 - 255
  int brake;   // 0 - 255
  int shift;   // bit 1: clutch in to shift up, bit 2: clutch in to shift dn

  roadObj_t obj;
} plrCtrls_t;


typedef struct gameState_t
{
  int timeLeft;
  plrCar_t plrCar;
  roadCtlr_t roadCtlr;
  plrCtrls_t plrCtrls;
  unsigned char kaThump;
  fixed_t hoodDuck;
  sfxState_t sfxState;

} gameState_t;


void gameInit(gameState_t *state);

void doGameLogic(gameState_t *state);

void doGameDraw(gameState_t *state, pcxImage_t *buf);

#endif

