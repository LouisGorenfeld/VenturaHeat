#include <stdio.h>
#include <string.h>
#include "game.h"
#include "lmath.h"
#include "draw.h"
#include "pcxfont.h"
#include "roadctlr.h"
#include "cars.h"

extern pcxFont_t gFont;


extern char gKeyDown[128];

#define MAXOFFROADSPEED 164
#define MAXSTEER 256
#define SPEEDLOSS 45000 // of 16.16 fixed point

pcxImage_t gCarHoodImg;

void gameInit(gameState_t *state)
{
  memset(state, 0, sizeof(gameState_t));
  roadCtlInit(&state->roadCtlr);
  initPlrCar(&state->plrCar);

  loadPCXFile(&gCarHoodImg, "data/carhood.pcx");
}

void doCarLogic(gameState_t *state) // TODO: Might end up in roadCtlr
{
/*
  int i;
  for (i=0; i < MAXCARS; i++)
  {
    if (state->ccars[i].logic)
       state->cars[i].logic();
  }
  */

}

void killCar(gameState_t *state)
{
  // TODO: Copy last car over the one to kill
}

void plrSteer(gameState_t *state, char amt)
{
  if (state->plrCar.car.speed == 0)
    return;
    
  state->plrCtrls.wheel += amt;
  if (state->plrCtrls.wheel < -127)
   state->plrCtrls.wheel = -127;
  else if (state->plrCtrls.wheel > 127)
   state->plrCtrls.wheel = 127;
}

void plrAutocenter(gameState_t *state, char spd)
{
  if (state->plrCtrls.wheel > 0)
  {
    state->plrCtrls.wheel -= spd;
    if (state->plrCtrls.wheel < 0)
      state->plrCtrls.wheel = 0;
  }
  else if (state->plrCtrls.wheel < 0)
  {
    state->plrCtrls.wheel += spd;
    if (state->plrCtrls.wheel > 0)
      state->plrCtrls.wheel = 0;
  }
}

void plrGas(gameState_t *state, unsigned char amt)
{
  state->plrCtrls.gas = amt;
  if (state->plrCtrls.gas > 255)
    state->plrCtrls.gas = 255;
}

void plrBrake(gameState_t *state, unsigned char amt)
{
  state->plrCtrls.brake = amt;
  if (state->plrCtrls.brake > 255)
    state->plrCtrls.brake = 255;
}

void plrStartShiftUp(gameState_t *state)
{
  state->plrCtrls.shift |= 1;
}

void plrStartShiftDn(gameState_t *state)
{
  state->plrCtrls.shift |= 2;
}

void plrEndShift(gameState_t *state)
{
  // Retrigger to restart that phasing
  triggerSfx(&state->sfxState, kSFX_Engine);

  if (state->plrCtrls.shift & 1)
  {
    state->plrCar.gear++;
    if (state->plrCar.gear >= state->plrCar.numGears)
      state->plrCar.gear = state->plrCar.numGears - 1;
  }

  if (state->plrCtrls.shift & 2)
  {
    state->plrCar.gear--;
    if (state->plrCar.gear < 0)
      state->plrCar.gear = 0;
  }

  state->plrCtrls.shift = 0;
}

void doGameInputs(gameState_t *state)
{
    if (gKeyDown[0x4b]) // left
      plrSteer(state, -4);
    else if (gKeyDown[0x4d]) // right
      plrSteer(state, 4);
    else
      plrAutocenter(state, 1);

    if (gKeyDown[0x48]) // up
      plrGas(state, 255);
    else
      plrGas(state, 0);

    if (gKeyDown[0x50]) // down
      plrBrake(state, 255);
    else
      plrBrake(state, 0);

    if (gKeyDown[0x1E]) // A (shift up)
      plrStartShiftUp(state);
    else if (gKeyDown[0x2C]) // Z (shift down)
      plrStartShiftDn(state);
    else
      plrEndShift(state);
}

void doCurvePhysics(gameState_t *state)
{
  plrCar_t *plrCar = &state->plrCar;
  roadLive_t *live = &state->roadCtlr.roadLive;
  fixed_t speed = plrCar->car.speed;
  fixed_t amount;

  amount = (getCurveAmount(live) * (FIXTOI(speed))) / 10000;
  
  // Push car outward according to curves
  if (amount)
   plrCar->car.roadObj.x += amount;
}

int rpmsForSpeed(plrCar_t *plrCar, int speed)
{
  gearDef_t *g = plrCar->gears + plrCar->gear;
  int speedRange = g->speedMax - g->speedMin;
  int rpmRange = MAXRPMS - MINRPMS;

  fixed_t x = (speed - ITOFIX(g->speedMin)) / speedRange;
  if (x < 0)
    x = 0;
//  else if (x >= 1<<FIXSHIFT)
//    x = 1<<FIXSHIFT;

  return MINRPMS + FIXTOI(x * rpmRange);
}

void carAccel(plrCar_t *plrCar, fixed_t mphIncrease, unsigned char gas)
{
  fixed_t amt = (mphIncrease * gas) >> 8;
  plrCar->car.speed += amt;
}

// TODO: When engaging the clutch after shifting, the RPMs are pulled towards
// the target value rather than snapping suddenly to it. That way the player
// also gets a slight surge as it's pulled down through the sweet spot.
void doGearBox(plrCar_t *car, plrCtrls_t *ctrls)
{
  unsigned char gas = ctrls->gas;
  gearDef_t *g = car->gears + car->gear;

  if (!ctrls->shift)
  {
    int speed = FIXTOI(car->car.speed);

    if (gas)
    {
      if (speed >= g->speedMin && speed < g->speedMax)
      {
        if (car->rpms < car->rpmsSweetLo)
          carAccel(car, g->accelGood, gas);
        else if (car->rpms < car->rpmsSweetHi)
          carAccel(car, g->accelSweet, gas);
        else if (car->rpms < car->rpmsBadLo)
          carAccel(car, g->accelGood, gas);
        else
          carAccel(car, g->accelBad, gas);
      }
      else
      {
        carAccel(car, g->accelBad, gas);
      }
    }

    int targetRPMs = rpmsForSpeed(car, car->car.speed);
    if (car->rpms > targetRPMs) 
    {
      car->rpms = .9 * car->rpms + .1 * targetRPMs;
    }
    else 
    {
      car->rpms = targetRPMs;
    }
  }
  else
  {
    car->rpms += (800 * gas) >> 9;
  }

  if (car->rpms >= OVERRPMS)
    car->rpms = OVERRPMS;

  // Engine drag
  if (car->car.speed >= ITOFIX(g->speedMax))
  {
    // TODO: The further over you are, the more it drags
    car->car.speed -= 1<<FIXSHIFT;
  }
  else
  {
    car->car.speed -= 1<<FIXSHIFT>>4;
  }
}

int isPlrOffroad(gameState_t *state)
{
  roadLive_t *live = &state->roadCtlr.roadLive;
  int rl = live->xFront - live->halfWidth;
  int rr = live->xFront + live->halfWidth;

  return (rl > -160 || rr < 160); // 160 is half screen width (TODO: use const)
}

void checkObjCollisions(gameState_t *state)
{
  // TODO: Handle being bumped from behind (if other car's speed is greater,
  // then it's from behind). Iterate through car list to find corresponding
  // roadObj
  // TODO: We may want to move the car's facing (ala steering) to
  // better simulate a skid
  roadObj_t *obj = state->roadCtlr.roadLive.objHead;
  while (obj)
  {
    if (obj->z > ZNEARPLANE) // List is sorted by z
      break;

    if (obj->sx >= 0 && obj->sx < 160)
    {
      // Hit left side
      state->plrCar.car.speed = FIXMUL(state->plrCar.car.speed, SPEEDLOSS);
      state->plrCar.skidding = -40;
      triggerSfx(&state->sfxState, kSFX_Crash);
    }
    else if (obj->sx >= 160 && obj->sx < 320)
    {
      // Hit right side
      state->plrCar.car.speed = FIXMUL(state->plrCar.car.speed, SPEEDLOSS);
      state->plrCar.skidding = 40;
      triggerSfx(&state->sfxState, kSFX_Crash);
    }

    obj = obj->next;
  }
}


void doGameLogic(gameState_t *state)
{
  int zScroll;
  plrCar_t *plrCar = &state->plrCar;
  doGameInputs(state);

  // TODO: There are still some lack-of-precision issues here, so perhaps
  // keep speed in MPH fixed point and save the rightshift

//  plrCar->car.speed = 30; // dbg
  zScroll = SPEEDTOZ(plrCar->car.speed);
  if (plrCar->car.speed > 0 && zScroll == 0)
    zScroll = 1;  // Don't let the lack of precision make the car inert
    
  roadCtlScroll(&state->roadCtlr, zScroll);
  roadCtlCarLogic(&state->roadCtlr);

  doGearBox(&state->plrCar, &state->plrCtrls);
  int freq = 50 + ((state->plrCar.rpms) >> 3);
  setSfxFrequency(&state->sfxState, kSFX_Engine, freq);

//  state->hoodDuck = state->plrCtrls.gas << 4;

  plrCar->car.speed -= state->plrCtrls.brake << 8;

  if (plrCar->car.speed < 0)
    plrCar->car.speed = 0;

  if (plrCar->skidding > 0)
  {
    plrCar->car.roadObj.x += 400;
    plrCar->skidding--;
  }
  else if (plrCar->skidding < 0)
  {
    plrCar->car.roadObj.x -= 400;
    plrCar->skidding++;
  }

  if (isPlrOffroad(state) || plrCar->car.speed == 0 /* rough idle */)
  {
    if (plrCar->car.speed > ITOFIX(MAXOFFROADSPEED))
      plrCar->car.speed -= ITOFIX(3);
    state->kaThump++;
  }
  else
  {
    state->kaThump = 0;
  }

  checkObjCollisions(state);

  doCurvePhysics(state);
  {
    fixed_t amt = (state->plrCtrls.wheel * (state->plrCar.car.speed >> 8)) >> 13;
    if (amt > MAXSTEER)
      amt = MAXSTEER;
    else if (amt < -MAXSTEER)
      amt = -MAXSTEER;
    plrCar->car.roadObj.x -= amt;
  }

  state->roadCtlr.roadLive.xBack = -state->plrCtrls.wheel >> 1;
  state->roadCtlr.roadLive.xFront = plrCar->car.roadObj.x >> 4;
  state->roadCtlr.roadLive.xHorizon = state->plrCtrls.wheel << 16;
}

void drawCrosshair(gameState_t *state, pcxImage_t *buf, int cx, int cy)
{
  vline(buf, cx, cy-2, cy-1, 15);
  vline(buf, cx, cy+2, cy+1, 15);
  hline(buf, cx-2, cy, cx-1, 15);
  hline(buf, cx+2, cy, cx+1, 15);
}

void drawHud(gameState_t *state, pcxImage_t *buf)
{
 // fixed_t y = (ITOFIX(105) - state->roadCtlr.roadLive.ddyFront);
 // drawCrosshair(state, buf, 160 + (state->plrCtrls.wheel >> 2), FIXTOI(y));
}

extern int gTicks;
void drawDebug(gameState_t *state, pcxImage_t *buf)
{
  char dbgline[80];
  roadLive_t *live = &state->roadCtlr.roadLive;
  int rl = live->xFront - (live->halfWidth);
  int rr = live->xFront + (live->halfWidth);

  hline(buf, 0, 0, 64, 0);
  hline(buf, 0, 1, 64, 1);
  buf->pixels[32+(state->plrCtrls.wheel>>2)] = 15;
//  buf[320+32+(state->plrCar.car.facing>>16)] = 15;


  sprintf(dbgline, "segPos=%d   speed=%d   gear=%d  z=%d    x=%d   rL=%d   rR=%d",
                   state->roadCtlr.roadLive.segPos>>16,
                   state->plrCar.car.speed>>16,
                   state->plrCar.gear,
                   state->roadCtlr.roadLive.z,
                   state->plrCar.car.roadObj.x,
                   rl, rr
  );
  blitFont(&gFont, buf->pixels, dbgline, 8, 8);

  // RPMs in bottom corner
  {
    plrCar_t *plr = &state->plrCar;
    gearDef_t *g = plr->gears + plr->gear;
    int speed = FIXTOI(plr->car.speed);
    int color;
    float width;

    if (speed >= g->speedMin && speed < g->speedMax)
    {
      if (plr->rpms < plr->rpmsSweetLo)
        color = 9;
      else if (plr->rpms < plr->rpmsSweetHi)
        color = 10;
      else if (plr->rpms < plr->rpmsBadLo)
        color = 9;
      else
        color = 4;
    }
    else if (speed >= g->speedMax)
    {
      color = 12;
    }
    else
    {
      color = 0;
    }

    width = ((float)(plr->rpms) / (float)MAXRPMS) * 64.0f;
    drawSquare(buf, 10, 200-32, 10+width, 200-24, color);
    vline(buf, 10+64+2, 200-32, 200-24, 15);
  }

//  hline(buf, 160-32, 200-50, 160+32, 0);
//  hline(buf, 160-32, 200-50+48, 160+32, 0);
//  drawSquare(buf, 160-32, 120, 160+32, 120+48, 0);
}

void doGameDraw(gameState_t *state, pcxImage_t *buf)
{
//  roadDraw(&state->roadCtlr.roadLive, buf, ((state->kaThump & 5) != 0));
  roadDraw(&state->roadCtlr.roadLive, buf, 0);
  drawHud(state, buf);
  blitPCXSprite(&gCarHoodImg, buf, 0,
                buf->height - gCarHoodImg.height - (state->kaThump&2) - FIXTOI(state->hoodDuck)
  );


  drawDebug(state, buf);
}


