#include <stdio.h>
#include <string.h>
#include "main.h"
#include "game.h"
#include "lmath.h"
#include "draw.h"
#include "pcxfont.h"
#include "roadctlr.h"
#include "cars.h"
#include "math.h"
#include "data.h"

extern pcxFont_t gFontTiny;
extern pcxImage_t gFontDigiImg; // TODO: pad this so we can use normal font routines
extern pcxFont_t gFontMedium;
extern pcxFont_t gFontDefault;
extern pcxImage_t gTachImg;
extern int gLastFPS;

#define MAXOFFROADSPEED 164
#define MAXSTEER 256
#define SPEEDLOSS FTOFIX(.25) // speed is multiplied against this in a crash; 16.16 fixed point

#define TIMESTART 70*70
#define TIMEBONUS 60*70

pcxImage_t gCarHoodImg;

void gameInit(gameState_t *state)
{
  memset(state, 0, sizeof(gameState_t));
  roadCtlInit(&state->roadCtlr);
  initPlrCar(&state->plrCar);
  state->timeLeft = TIMESTART;
  state->gameOverCountdown = -1;

  loadPCX(&gCarHoodImg, data_carhood);
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

  if (amt > 127 && state->plrCar.car.speed > 0)
    triggerSfx(&state->sfxState, kSFX_Skid);

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
      plrSteer(state, -3);
    else if (gKeyDown[0x4d]) // right
      plrSteer(state, 3);
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

  amount = (getCurveAmount(live) * (FIXTOI(speed))) / 8000; //10000;
  
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

    // When engaging the clutch after shifting, the RPMs are pulled towards
    // the target value rather than snapping suddenly to it.
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

void doCarGeneration(gameState_t *state) {

  if (state->plrCar.car.speed < ITOFIX(150))
    return;

    // Find most-distant car. If speed==0, we know it's a tree, sign, special object, etc
  roadCtlr_t *ctlr = &state->roadCtlr;
  roadObj_t *pObj = ctlr->roadLive.objTail;
  while (pObj && pObj->car && pObj->car->speed == 0) {
    pObj = pObj->last;
  }

  int genTraffic = 0;
  if (pObj) {
    int distanceFromEnd = ZFARPLANE - pObj->z;
    if (distanceFromEnd >= ctlr->zTrafficDelta) {
      genTraffic = 1;
    }
  }
  else {  // No moving cars found on playfield; generate one
      genTraffic = 1;
  }

  if (genTraffic) {
    if (state->roadCtlr.curStage == 0) {
      ctlr->zTrafficDelta = 2048 + (rand() & 1023);
    }
    else if (state->roadCtlr.curStage == 1) {
      ctlr->zTrafficDelta = 2048 + (rand() & 511);
    }
    else if (state->roadCtlr.curStage == 3) {
      ctlr->zTrafficDelta = 512 + (rand() & 511);
    }
    else {
      ctlr->zTrafficDelta = 1024 + (rand() & 511);
    }
    roadCtlrAddRandomizedCar(ctlr, 0);
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
  const int kSkidAmount = 20;

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

    if (obj->car->type != CARTYPE_DUMMY_SCEN) 
    {
      if (obj->sx >= 0 && obj->sx < 160 && !state->plrCar.skidding)
      {
        // Hit left side
        state->plrCar.car.speed = FIXMUL(state->plrCar.car.speed, SPEEDLOSS);
        state->plrCar.skidding = -kSkidAmount;
        triggerSfx(&state->sfxState, kSFX_Crash);
      }
      else if (obj->sx >= 160 && obj->sx < 320 && !state->plrCar.skidding)
      {
        // Hit right side
        state->plrCar.car.speed = FIXMUL(state->plrCar.car.speed, SPEEDLOSS);
        state->plrCar.skidding = kSkidAmount;
        triggerSfx(&state->sfxState, kSFX_Crash);
      }
    }

    obj = obj->next;
  }
}

void doGameLogic(void *s)
{
  gameState_t *state = (gameState_t*)s;
  if (state->crossedFinish)
    return;

  plrCar_t *plrCar = &state->plrCar;

  if (state->gameOverCountdown > 0) {
    state->gameOverCountdown--;
    return;
  }

  doGameInputs(state);

  // TODO: There are still some lack-of-precision issues here, so perhaps
  // keep speed in MPH fixed point and save the rightshift


  state->curLaptime++;
  state->totaltime++;

//  plrCar->car.speed = 30; // dbg
  int zScroll = SPEEDTOZ(plrCar->car.speed);
  if (plrCar->car.speed > 0 && zScroll == 0)
    zScroll = 1;  // Don't let the lack of precision make the car inert
    
  int nextStageLoaded = roadCtlScroll(&state->roadCtlr, zScroll);
  if (nextStageLoaded) {
    state->laptimes[state->roadCtlr.curStage - 1] = state->curLaptime;
    state->curLaptime = 0;

    if(state->roadCtlr.curStage == MAX_STAGES) {
      state->crossedFinish = 1;
      return; // Early return and don't call us back
    }
    else {
      triggerSfx(&state->sfxState, kSFX_Extend);
      state->msg = "TIME EXTENDED";
      state->msgBlinkMask = 1;
      state->msgCountdown = 70*2;
      state->timeLeft += TIMEBONUS;
      if (state->timeLeft > TIMESTART)
        state->timeLeft = TIMESTART;
    }
  }
  roadCtlCarLogic(&state->roadCtlr);

  doGearBox(&state->plrCar, &state->plrCtrls);
  int freq = 50 + ((state->plrCar.rpms) >> 3);
  freq += rand() & 0x1f;
  setSfxFrequency(&state->sfxState, kSFX_Engine, freq);

//  state->hoodDuck = state->plrCtrls.gas << 4;

  plrCar->car.speed -= state->plrCtrls.brake << 8;

  if (plrCar->car.speed < 0)
    plrCar->car.speed = 0;

  if (plrCar->skidding > 0)
  {
    plrCar->car.roadObj.x += 400;
    plrCar->skidding--;
    triggerSfx(&state->sfxState, kSFX_Skid);
  }
  else if (plrCar->skidding < 0)
  {
    plrCar->car.roadObj.x -= 400;
    plrCar->skidding++;
    triggerSfx(&state->sfxState, kSFX_Skid);
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

  fixed_t xBefore = plrCar->car.roadObj.x;
  fixed_t amt = (state->plrCtrls.wheel * (state->plrCar.car.speed >> 8)) >> 13;
  if (amt > MAXSTEER)
    amt = MAXSTEER;
  else if (amt < -MAXSTEER)
    amt = -MAXSTEER;
  plrCar->car.roadObj.x -= amt;

  doCurvePhysics(state);

  // Detect whether the player is turning hard against a curve
  if (state->plrCtrls.wheel < -64) { // Turning left, which increases x (left side is +)
    if (plrCar->car.roadObj.x <= xBefore) {
      triggerSfx(&state->sfxState, kSFX_Skid);
    }
  }
  else if (state->plrCtrls.wheel > 64) { // Turning right, which decreases x (right side is -)
    if (plrCar->car.roadObj.x >= xBefore) {
      triggerSfx(&state->sfxState, kSFX_Skid);
    }
  }

  state->roadCtlr.roadLive.xBack = -state->plrCtrls.wheel >> 1;
  state->roadCtlr.roadLive.xFront = plrCar->car.roadObj.x >> 4;
  state->roadCtlr.roadLive.xHorizon = state->plrCtrls.wheel << 16;

  doCarGeneration(state);

  if (state->timeLeft > 0) {
    state->timeLeft--;
    if (state->timeLeft == 10*70) {
      state->msg = "TIME IS RUNNING OUT";
      state->msgCountdown = 70*2;
      state->msgBlinkMask = 1;
    }
  }
  else if (state->timeLeft == 0) {
    state->msg = "GAME OVER";
    state->msgBlinkMask = 0;
    state->msgCountdown = 70*3;
    state->gameOverCountdown = state->msgCountdown;
  }

  tickSfx(&state->sfxState);

  if (state->msgCountdown > 0)
    state->msgCountdown--;
}

void drawCrosshair(gameState_t *state, pcxImage_t *buf, int cx, int cy)
{
  vline(buf, cx, cy-2, cy-1, 15);
  vline(buf, cx, cy+2, cy+1, 15);
  hline(buf, cx-2, cy, cx-1, 15);
  hline(buf, cx+2, cy, cx+1, 15);
}


extern int gLastFPS;
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
/*

/*
  sprintf(dbgline, "segPos=%d   speed=%d   gear=%d  z=%d    x=%d   rL=%d   rR=%d",
                   state->roadCtlr.roadLive.segPos>>16,
                   state->plrCar.car.speed>>16,
                   state->plrCar.gear,
                   state->roadCtlr.roadLive.z,
                   state->plrCar.car.roadObj.x,
                   rl, rr
  );*/

  sprintf(dbgline, "FPS: %d", gLastFPS);
  blitFont(&gFontTiny, buf->pixels, dbgline, 8, 8);

  // RPMs in bottom corner
  #if 0
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
#endif
//  hline(buf, 160-32, 200-50, 160+32, 0);
//  hline(buf, 160-32, 200-50+48, 160+32, 0);
//  drawSquare(buf, 160-32, 120, 160+32, 120+48, 0);
}

void drawDigiNum(pcxImage_t *dblBuf, pcxImage_t *numImg, int num, int x, int y)
{
  char str[128];
  sprintf(str, "%d", num);  
  char *p = str;
  int wx = x;
  while (*p) {
    const int rx = 24 * (*p - '0');
    blitPCXClip(numImg, dblBuf, wx, y, rx, 0, rx + 24, numImg->height);
    wx += 24;
    p++;
  }
}

void showTitle();
void doGameDraw(void *s, pcxImage_t *buf)
{
  gameState_t *state = (gameState_t*)s;
  if (state->gameOverCountdown == 0) {
    return;
  }

  roadDraw(&state->roadCtlr.roadLive, buf, 0, 200, 200);
  blitPCXSprite(&gCarHoodImg, buf, 0,
                buf->height - gCarHoodImg.height - (state->kaThump&2) - FIXTOI(state->hoodDuck)
  );
  if (state->msg && state->msgCountdown > 0 && (state->msgCountdown & state->msgBlinkMask)) {
    int x = (320 - (strlen(state->msg)*gFontMedium.fontWidth)) >> 1;
    blitFont(&gFontMedium, buf->pixels, state->msg, x, 70);
  }

  // Draw the HUD
  drawSquare(buf, 0, 0, 320, 32, 0);
  int cx = (float)gTachImg.width * (float)state->plrCar.rpms / (float)OVERRPMS;
  if (cx) {
    blitPCXClip(&gTachImg, buf, 24*4, 8, 0, 0, cx, gTachImg.height);
  }

  // TODO: We really might want to make a compatible font ..
  drawDigiNum(buf, &gFontDigiImg, FIXTOI(state->plrCar.car.speed), 0, 4);
  drawDigiNum(buf, &gFontDigiImg, state->plrCar.gear + 1, 200, 4);
  drawDigiNum(buf, &gFontDigiImg, state->timeLeft/70, 250, 4);
}


