#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "roadctlr.h"
#include "game.h"
#include "gfxstor.h"

#define ZSCENSPACING (ZFARPLANE/10)
#define ZTRAFSPACING (ZFARPLANE)

#define LIGHTCURVE 6000 //7000
#define MEDCURVE 9000 //11000
#define SHARPCURVE 12000
#define LIGHTHILL 100
#define MEDHILL 500
#define SHARPHILL 1000

// HORIZON NEEDS TO RISE/SINK MORE TO SELL THE ILLUSION

char *parseTexChange(roadCtlr_t *ctlr, char *ch);

void roadCtlInit(roadCtlr_t *ctlr)
{
  //TEST:
//  char *testStr = "UT(11),lU,Ld,ru,D,d,rD,RD,U,_,l,l,H,r,ld,u,D,U,d,U,U,rd,lD,D,ru,ld,RU,R,r,RD,_l,r,l,L,_,u,dr,_,D,u,d,u,L,l,l,R,L,R,r,u,H,U,d,D,r,d,_,l,l,L,_,Ur,u,D,d,_,l,l,u,U,_,_,d,l,D,d,r,_,_,l,r,r,_,_,l,R,r,r,_,_,r,R,L,L,L,_,_,r,r,R,r,_,_,_,R,l,_,_,_,_,_,_,_";
//  char *testStr = "U,lU,Ld,ru,D,d,rD,RD,U,_,l,l,H,r,ld,u,D,U,d,U,U,rd,lD,D,ru,ld,RU,R,r,RD,_l,r,l,L,_,u,dr,_,D,u,d,u,L,l,l,R,L,R,r,u,H,U,d,D,r,d,_,l,l,L,_,Ur,u,D,d,_,l,l,u,U,_,_,d,l,D,d,r,_,_,l,r,r,_,_,l,R,r,r,_,_,r,R,L,L,L,_,_,r,r,R,r,_,_,_,R,l,_,_,_,_,_,_,_";
  char *testStr = "L,R,_,U,D,l,_,T(00),l,r,T(10),u,d,r,d,u,l,R,D,U,l,d,_,d,l_l,ur,d,_,u,R,r,_,l,dr,d,U,D,_rRRLl,uR,dL,H,r";

  ctlr->stage[0].cmd = "_,_,l,r,_,ur,d,l,_,_,r,dr,u,l,_,_,_,_";
  ctlr->stage[1].cmd = "U,D,T(02),_,l,u,dr,_,_,l,r,L,u,d,r,_,_,ru"; 
  ctlr->stage[2].cmd = "T(11)rd,T(01)_,rT(21),R,r,_,_,U,D,r,r,LT(10),T(02),l,uR,Dr,u";
  
  ctlr->parsePos = ctlr->stage[0].cmd;

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
  side = *ch - '0' - 1;
  ch++;
  assert(*ch >= '0' && *ch <= '9');
  tex = *ch - '0';
  ch++;
  assert(*ch == ')');

  roadCtlAddCar(ctlr, side, ZFARPLANE-1, 0, CARTYPE_DUMMY_SCEN, tex);

  return ch;
}

void loadNextStage(roadCtlr_t *ctlr) 
{
  if (ctlr->curStage < MAX_STAGES - 1) {
    ctlr->curStage++;
  }
  ctlr->parsePos = ctlr->stage[ctlr->curStage].cmd;
  // TODO: background? :)
}

void parseCommand(roadCtlr_t *ctlr)
{
  char *ch = ctlr->parsePos;
  roadLive_t *live = &ctlr->roadLive;

  live->ddxFront = live->ddxSeg;
  live->ddyFront = live->ddySeg;

  // Execute command delineated by commas
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
    }

    ch++;
    if (*ch == ',')
    {
      ch++;
      break;
    }
    else if (*ch == 0)
    {
      // TODO: Time extend callback into gamestate
      loadNextStage(ctlr);
      ch = ctlr->parsePos;
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

  yOffsetDest = 0; // TODO: Maybe we want to use this again

  live->texOff =(live->texOff + zAmt) & ((1<<ZMAPBITS)-1);
  live->z += zAmt;
  live->segPos += zAmt<<10; // below 8 shaves off so much that it never moves
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
  ctlr->zCounterScenery -= zAmt;
  if (ctlr->zCounterScenery < 0)
  {
    char nextCmd = peekCurveAhead(ctlr);
    char curCmd = *(ctlr->parsePos);

    ctlr->zCounterScenery += ZSCENSPACING;

    // There's a specific priority here, where the upcoming curve
    // gets priority over signs describing the current curve
    if (nextCmd == 'L' || nextCmd == 'h')
    {
        roadCtlAddCar(ctlr, 85536, ZFARPLANE-1, 0, CARTYPE_LCHEV, -1);
    }
    else if (nextCmd == 'R' || nextCmd == 'H')
    {
        roadCtlAddCar(ctlr, -85536, ZFARPLANE-1, 0, CARTYPE_RCHEV, -1);
    }
    else if (curCmd == 'L' || curCmd == 'h') 
    {
        roadCtlAddCar(ctlr, 85536, ZFARPLANE-1, 0, CARTYPE_LCHEV, -1);
    }
    else if (curCmd == 'R' || curCmd == 'H') 
    {
        roadCtlAddCar(ctlr, -85536, ZFARPLANE-1, 0, CARTYPE_RCHEV, -1);
    }
    else
    {
      // Randomized trees
      if (rand() & 1)
        roadCtlAddCar(ctlr, 85536 + (rand() & 131071), ZFARPLANE-1, 0, CARTYPE_TREE, -1);
      else
        roadCtlAddCar(ctlr, -85536 - (rand() & 131071), ZFARPLANE-1, 0, CARTYPE_TREE, -1);
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
      {
        // If it's a scenery change, then the scenery at the front of the screen
        // becomes this one
        if (car->type == CARTYPE_DUMMY_SCEN)
        {
          if (car->roadObj.x == 0)
            ctlr->roadLive.roadTex = car->roadObj.img;
          else if (car->roadObj.x == -1)
            ctlr->roadLive.sideTex[0] = car->roadObj.img;
          else if (car->roadObj.x == 1)
            ctlr->roadLive.sideTex[1] = car->roadObj.img;
        } 
        
        roadCtlRemoveCar(ctlr, car);
      }
    }
  }
}

void roadCtlrAddRandomizedCar(roadCtlr_t *ctlr, int rank)
{
  const fixed_t third = ITOFIX(1)/3;
  const fixed_t laneXs[] = {
    -32767,
    0,
    32767
  };

  // Make sure the farthest car's z >= minZdistance from the end
  fixed_t speed = ITOFIX(175);// ITOFIX((rank+1) * 65 + 5);

  // Roll one of these patterns for the 3 lanes:
  // 1 2 3
  // -
  //   -
  //     -
  // -   - <-- staggered
    int pattern = rand() & 3;
    int texNum = (rand() & 1)? 1 : 4;
    car_t *car;
    speed += ITOFIX(rand() & 31);
    if (pattern == 3) {
        fixed_t z = ZFARPLANE-1;
        car = roadCtlAddCar(ctlr, laneXs[0], z, speed, CARTYPE_COUPE, texNum);
        car =roadCtlAddCar(ctlr, laneXs[2], z - 512, speed, CARTYPE_COUPE, texNum);
//      roadCtlAddCar(ctlr, 0, z, speed, CARTYPE_COUPE);
    }
    else {
        const int lane = pattern;      
        car =roadCtlAddCar(ctlr, laneXs[lane], ZFARPLANE-1, speed, CARTYPE_COUPE, texNum);
//      roadCtlAddCar(ctlr, 0, z, speed, CARTYPE_COUPE);
    }

  // If rank is > 3, you start getting ambulances in the right lane
  // Prereq: rear view mirror 
} 

void roadCtlRollEvent(roadCtlr_t, int rank)
{
  // events:
  // Left lane out (maybe becomes water..? construction?)
  // Right lane out
}

// TODO We might want to wrap this in addScenaryObject and addCarObject and this becomes addRoadObject
car_t *roadCtlAddCar(roadCtlr_t *ctlr, fixed_t x, int z, fixed_t speed, carType_t type, int texNum)
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
  car->roadObj.car = car;   // Make sure we can get back to ourself easily

  // TODO: Texnum for cars can be transformed into texNum*3, for texNum + offset, where offset
  // is 0=left-angled, 1=centered, 2=right-angled
  initCar(car, x, z, speed, type, texNum);
  if (type != CARTYPE_DUMMY_SCEN) {
    if (type == CARTYPE_LCHEV)
      car->roadObj.img = getCarTex(car->texNum, CARGFX_LEFT); 
    else if (type == CARTYPE_RCHEV)
      car->roadObj.img = getCarTex(car->texNum, CARGFX_RIGHT);
    else
      car->roadObj.img = getCarTex(car->texNum, CARGFX_CENTER);
  }
  else {
    if (x == 0)
      car->roadObj.img = getRoadTex(car->texNum);
    else 
      car->roadObj.img = getSideTex(car->texNum);
  }

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



