#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "roadctlr.h"
#include "game.h"
#include "gfxstor.h"

#define ZSCENSPACING (ZFARPLANE/10)

#define LIGHTCURVE 9000 //7000
#define MEDCURVE 12000 //11000
#define SHARPCURVE 14000
#define LIGHTHILL 100
#define MEDHILL 500
#define SHARPHILL 1000

// HORIZON NEEDS TO RISE/SINK MORE TO SELL THE ILLUSION

void roadCtlInit(roadCtlr_t *ctlr)
{
  //TEST:
//  char *testStr = "UT(11),lU,Ld,ru,D,d,rD,RD,U,_,l,l,H,r,ld,u,D,U,d,U,U,rd,lD,D,ru,ld,RU,R,r,RD,_l,r,l,L,_,u,dr,_,D,u,d,u,L,l,l,R,L,R,r,u,H,U,d,D,r,d,_,l,l,L,_,Ur,u,D,d,_,l,l,u,U,_,_,d,l,D,d,r,_,_,l,r,r,_,_,l,R,r,r,_,_,r,R,L,L,L,_,_,r,r,R,r,_,_,_,R,l,_,_,_,_,_,_,_";
//  char *testStr = "U,lU,Ld,ru,D,d,rD,RD,U,_,l,l,H,r,ld,u,D,U,d,U,U,rd,lD,D,ru,ld,RU,R,r,RD,_l,r,l,L,_,u,dr,_,D,u,d,u,L,l,l,R,L,R,r,u,H,U,d,D,r,d,_,l,l,L,_,Ur,u,D,d,_,l,l,u,U,_,_,d,l,D,d,r,_,_,l,r,r,_,_,l,R,r,r,_,_,r,R,L,L,L,_,_,r,r,R,r,_,_,_,R,l,_,_,_,_,_,_,_";
  char *testStr = "U,D,u,D,l,d,_,d,l_l,ur,d,_,u,R,r,_,l,dr,d,U,D,_";
  ctlr->stage.cmd = testStr;
  ctlr->parsePos = ctlr->stage.cmd;

  roadInit(&ctlr->roadDef, &ctlr->roadLive);

  roadCtlSetTexture(ctlr, TEXLOC_ROAD, getRoadTex(0));
  roadCtlSetTexture(ctlr, TEXLOC_LEFT, getSideTex(0));
  roadCtlSetTexture(ctlr, TEXLOC_RIGHT, getSideTex(0));

  // TODO: Hack for now. Horizon location will change anyway
  ctlr->roadDef.horizonTex = getHorizonTex(0);
  ctlr->roadLive.yOffsetHorizon = ITOFIX(ctlr->roadDef.horizonTex->height >> 2);
}

char *parseTexChange(roadCtlr_t *ctlr, char *ch)
{
  int side;
  int tex;
  ch++;
  assert(*ch == '(');
  ch++;
  assert(*ch >= '0' && *ch <= '9');
  side = *ch - 0;
  ch++;
  assert(*ch >= '0' && *ch <= '9');
  tex = *ch - 0;
  ch++;
  assert(*ch == ')');

  roadCtlAddCar(ctlr, side, ZFARPLANE-1, 0, CARTYPE_DUMMY_SCEN);

  return ch;
}

void parseCommand(roadCtlr_t *ctlr)
{
  char *ch = ctlr->parsePos;
  roadLive_t *live = &ctlr->roadLive;

  live->ddxFront = live->ddxSeg;
  live->ddyFront = live->ddySeg;


  while (*ch != 0)
  {
    switch (*ch)
    {
      case 'l':  live->ddxSeg = -LIGHTCURVE;   break;
      case 'L':  live->ddxSeg = -MEDCURVE;     break;
      case 'r':  live->ddxSeg = LIGHTCURVE;    break;
      case 'R':  live->ddxSeg = MEDCURVE;      break;
      case 'h':  live->ddxSeg = -SHARPCURVE;   break;
      case 'H':  live->ddxSeg = SHARPCURVE;    break;
      case 'u':  live->ddySeg = MEDHILL;       break;
      case 'U':  live->ddySeg = SHARPHILL;     break;
      case 'd':  live->ddySeg = -LIGHTHILL;    break;
      case 'D':  live->ddySeg = -MEDHILL;      break;
      case '_':  live->ddxSeg = 0;
                 live->ddySeg = 0;
                 break;
      case 'T':  ch = parseTexChange(ctlr, ch);
                 break;
      case 0:
        exit(9);  // End of the road
    }

    ch++;
    if (*ch == ',')
    {
      ch++;
      break;
    }
  }

  ctlr->parsePos = ch;
}


// Return the curve command if there is one in the next command.
// Otherwise, return null if it's just straight.
char peekCurveAhead(roadCtlr_t *ctlr)
{
  char *ch = ctlr->parsePos;

  while (*ch != 0)
  {
    switch (*ch)
    {
      case 'l':
      case 'L':
      case 'r':
      case 'R':
      case 'h':
      case 'H':
           return *ch;
    }

    ch++;
    if (*ch == ',')
      break;
  }

  return 0;
}

void roadCtlScroll(roadCtlr_t *ctlr, short zAmt)
{
  int i;
  roadLive_t *live = &ctlr->roadLive;
  fixed_t xScrollAmount = (getCurveAmount(live) * (fixed_t)zAmt) << 0;
  fixed_t yScrollAmount = -(getHillAmount(live) * (fixed_t)zAmt) >> 3;
  fixed_t yOffsetDest = -getHillAmount(live) << 8;

  // TEMP: yScroll is disabled
  yOffsetDest = 0; // TODO: Maybe we want to use this again
  yScrollAmount = 0;

  live->texOff =(live->texOff + zAmt) & ((1<<ZMAPBITS)-1);
  live->z += zAmt;
  live->segPos += zAmt<<9;//10; // below 8 shaves off so much that it never moves
  if (live->segPos >= ZMAPLEN<<16)
  {
    live->segPos -= ZMAPLEN<<16;
    parseCommand(ctlr);
  }

  // TODO: We may want a CarCtlr if car logic starts dominating this module
  for (i=0; i < MAXCARS; i++)
  {
    if (ctlr->carActive[i])
    {
      car_t *car = ctlr->cars + i;
      car->roadObj.z -= zAmt;
// Duplicate culling!
//      if (car->roadObj.z <= ZNEARCULL || car->roadObj.z >= ZFARCULL)
//        roadCtlRemoveCar(ctrl, car);
    }
  }


  // Put down scenery or chevrons
  ctlr->zCounter -= zAmt;
  if (ctlr->zCounter < 0)
  {
    char nextCmd = peekCurveAhead(ctlr);

    ctlr->zCounter += ZSCENSPACING;

    if (nextCmd == 'L' || nextCmd == 'h')
    {
        roadCtlAddCar(ctlr, 85536, ZFARPLANE-1, 0, CARTYPE_LCHEV);
    }
    else if (nextCmd == 'R' || nextCmd == 'H')
    {
        roadCtlAddCar(ctlr, -85536, ZFARPLANE-1, 0, CARTYPE_RCHEV);
    }
    else
    {
      // Randomized trees
      if (rand() & 1)
        roadCtlAddCar(ctlr, 85536 + (rand() & 131071), ZFARPLANE-1, 0, CARTYPE_TREE);
      else
        roadCtlAddCar(ctlr, -85536 - (rand() & 131071), ZFARPLANE-1, 0, CARTYPE_TREE);
    }

  }
  
  // Scroll horizon based on speed and turn sharpness
  live->xHorizon += xScrollAmount;

  if (live->yOffsetHorizon < yOffsetDest)
  {
    live->yOffsetHorizon += yScrollAmount;
    if (live->yOffsetHorizon > yOffsetDest)
       live->yOffsetHorizon = yOffsetDest;
  }
  else if (live->yOffsetHorizon > yOffsetDest)
  {
    live->yOffsetHorizon -= yScrollAmount;
    if (live->yOffsetHorizon < yOffsetDest)
       live->yOffsetHorizon = yOffsetDest;
  }  
}

void roadCtlSetTexture(roadCtlr_t *ctrl, texLoc_t loc, pcxImage_t *img)
{
  // Eventually these will start at maximum z and come towards
  // the camera. For now, the road just supports changing the texture
  // in its entirety
  switch (loc)
  {
    case TEXLOC_LEFT:   ctrl->roadLive.sideTex[0] = img;    break;
    case TEXLOC_ROAD:   ctrl->roadLive.roadTex = img;       break;
    case TEXLOC_RIGHT:  ctrl->roadLive.sideTex[1] = img;    break;
    default:
            assert(0);
  }
}

void roadCtlCarLogic(roadCtlr_t *ctlr)
{
  int i;
  for (i=0; i < MAXCARS; i++)
  {
    if (ctlr->carActive[i])
    {
      car_t *car = ctlr->cars + i;
      carLogic(ctlr, car);

      // Cull if outside z range
      if (car->roadObj.z <= 0 || car->roadObj.z >= ZFARCULL)
        roadCtlRemoveCar(ctlr, car);
    }
  }


//  // Roll for new cars
  if ((rand() & 255) == 0)
    roadCtlAddCar(ctlr, (rand() & 65535) - 32767, ZFARPLANE-1, ITOFIX(155), CARTYPE_COUPE);
}

car_t *roadCtlAddCar(roadCtlr_t *ctlr, fixed_t x, int z, fixed_t speed, carType_t type)
{
  int i;
  car_t *car;

  // Find next free spot
  for (i=0; i < MAXCARS; i++)
  {
    if (ctlr->carActive[i] == 0)
      break;
  }

  assert(i != MAXCARS);
  if (i == MAXCARS)
    return 0;
  
  car = ctlr->cars + i;

  initCar(car, x, z, speed, type);
  if (car->texNum >= 0)
    car->roadObj.img = getCarTex(car->texNum, CARGFX_CENTER);
  else
    car->roadObj.img = 0; //(pcxImage_t*)car->texNum; // typically -1 for dummy objects
  

  // Add it to the display list
  linkRoadObj(&ctlr->roadLive, &car->roadObj);
  ctlr->carActive[i] = 1;
  
  return car;
}

void roadCtlRemoveCar(roadCtlr_t *ctlr, car_t *car)
{
  int i;
  car_t *cars = ctlr->cars;

  for (i=0; i < MAXCARS; i++)
  {
    if (cars + i == car)
    {
      unlinkRoadObj(&ctlr->roadLive, &car->roadObj);
      ctlr->carActive[i] = 0;;
      
      return;
    }
  }

  // It should never be the case that a object requesting removal
  // is not in the list
  assert(0);
}



