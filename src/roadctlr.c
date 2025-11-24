#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "roadctlr.h"
#include "game.h"
#include "gfxstor.h"

#define ZSCENSPACING (ZFARPLANE/10)
#define ZTRAFSPACING (ZFARPLANE)

#define LIGHTCURVE 5000 //7000
#define MEDCURVE 8000 //11000
#define SHARPCURVE 10000
#define LIGHTHILL 100
#define MEDHILL 300
#define SHARPHILL 1000

// HORIZON NEEDS TO RISE/SINK MORE TO SELL THE ILLUSION

char *parseTexChange(roadCtlr_t *ctlr, char *ch);

void roadCtlInit(roadCtlr_t *ctlr)
{
  /* REFERENCE:
    // Road Textures
  loadPCX(stor->roadTex + 0, data_roadtex3);
  loadPCX(stor->roadTex + 1, data_iceroad);
  loadPCX(stor->roadTex + 2, data_chektex);



  // Side Textures
  loadPCX(stor->sideTex + 0, data_grastex1);
  loadPCX(stor->sideTex + 1, data_snowtex1);
  loadPCX(stor->sideTex + 2, data_beactex1);
  loadPCX(stor->sideTex + 3, data_beactex2);
  loadPCX(stor->sideTex + 4, data_mudtex1);


*/
  //TEST:
//  char *testStr = "UT(11),lU,Ld,ru,D,d,rD,RD,U,_,l,l,H,r,ld,u,D,U,d,U,U,rd,lD,D,ru,ld,RU,R,r,RD,_l,r,l,L,_,u,dr,_,D,u,d,u,L,l,l,R,L,R,r,u,H,U,d,D,r,d,_,l,l,L,_,Ur,u,D,d,_,l,l,u,U,_,_,d,l,D,d,r,_,_,l,r,r,_,_,l,R,r,r,_,_,r,R,L,L,L,_,_,r,r,R,r,_,_,_,R,l,_,_,_,_,_,_,_";
//  char *testStr = "U,lU,Ld,ru,D,d,rD,RD,U,_,l,l,H,r,ld,u,D,U,d,U,U,rd,lD,D,ru,ld,RU,R,r,RD,_l,r,l,L,_,u,dr,_,D,u,d,u,L,l,l,R,L,R,r,u,H,U,d,D,r,d,_,l,l,L,_,Ur,u,D,d,_,l,l,u,U,_,_,d,l,D,d,r,_,_,l,r,r,_,_,l,R,r,r,_,_,r,R,L,L,L,_,_,r,r,R,r,_,_,_,R,l,_,_,_,_,_,_,_";
  char *testStr = "L,R,_,U,D,l,_,T(00),l,r,T(10),u,d,r,d,u,l,R,D,U,l,d,_,d,l_l,ur,d,_,u,R,r,_,l,dr,d,U,D,_rRRLl,uR,dL,H,r";

  ctlr->stage[0].cmd = "_,r,_,l,u,d,r,d,_,_,r,ur,r,_,dl,_,ur,dl,_,_,_";
  ctlr->stage[1].cmd = "T(21),l,r,_,ur,d,_,l,u,d,r,d,dr,u,rD,l,_,_,U,D";
  ctlr->stage[2].cmd = "T(02),d,l,u,dr,u,d,_,l,r,L,L,u,d,r,R,ru,u,_,_,_"; 

  ctlr->stage[3].cmd = "T(03),T(23),_,_,_,u,_,l,l,d,_,d,d,u,l,r,_,_,_";

  ctlr->stage[4].cmd = "T(11),rd,T(01)_,rT(21),R,r,_,_,U,D,r,H,LT(10),T(02),l,uR,Ur,D";
  ctlr->stage[5].cmd = "T(04),T(00),T(24)l,l,uL,dL,L,R,u,l,d,_,L,h,U,l,R,uR,ul,_,_";
  ctlr->stage[6].cmd = "T(02),T(11),H,h,T(24),u,d,r,Ru,rd,H,_,L,lU,D,l,R,uR,ul,T(12)_";
//  ctlr->stage[6].cmd = "_,_,T(12)_"; // for testing finish line

  ctlr->curStage = 0;
  ctlr->parsePos = ctlr->stage[ctlr->curStage].cmd;

  roadInit(&ctlr->roadDef, &ctlr->roadLive);

  roadCtlSetTexture(ctlr, TEXLOC_ROAD, getRoadTex(0));
  roadCtlSetTexture(ctlr, TEXLOC_LEFT, getSideTex(0));
  roadCtlSetTexture(ctlr, TEXLOC_RIGHT, getSideTex(0));

  ctlr->scenIndex1[0] = 2;  // Left side
  ctlr->scenIndex2[0] = 3;
  ctlr->scenIndex1[1] = -1; // No randomized features on road yet
  ctlr->scenIndex2[1] = -1;
  ctlr->scenIndex1[2] = 2;  // Right side
  ctlr->scenIndex2[2] = 3;
  
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
  side = *ch - '0' - 1; // -1, 0, 1 as roadCtlAddCar expects
  // TODO, maybe just use 0 1 2 for everything?
  ch++;
  assert(*ch >= '0' && *ch <= '9');
  tex = *ch - '0';
  ch++;
  assert(*ch == ')');

  roadCtlAddCar(ctlr, side, ZFARPLANE-1, 0, CARTYPE_DUMMY_SCEN, tex);

  // Switch scenery over immediately, based on texture number:
  // 0 = grass
  // 1 = ice
  // 2 = beach
  // 3 = sand
  // 4 = mud
  if (tex == 0) {
    // Bushes and bare tree    
    ctlr->scenIndex1[side + 1] = 2;
    ctlr->scenIndex2[side + 1] = 3;
  }
  else if (tex == 1) {
    // Bare tree
    ctlr->scenIndex1[side + 1] = 2;
    ctlr->scenIndex2[side + 1] = 2;
  }
  else if (tex == 2) {
    // No beach scenery
    ctlr->scenIndex1[side + 1] = -1;
    ctlr->scenIndex2[side + 1] = -1;
  }
  else if (tex == 3) {
    // Cactus and bushes
    ctlr->scenIndex1[side + 1] = 5;
    ctlr->scenIndex2[side + 1] = 3;
  }
  else if (tex == 4) {
    // Only bushes
    ctlr->scenIndex1[side + 1] = 3;
    ctlr->scenIndex2[side + 1] = 3;
  }
  else {
    ctlr->scenIndex1[side + 1] = -1;
    ctlr->scenIndex2[side + 1] = -1;
  }
  

  return ch;
}

char *loadNextStage(roadCtlr_t *ctlr) 
{
  ctlr->curStage++;
  if (ctlr->curStage < MAX_STAGES)
    return ctlr->stage[ctlr->curStage].cmd;
  else  // Loop last stage
    return ctlr->stage[MAX_STAGES-1].cmd;
}

int parseCommand(roadCtlr_t *ctlr)
{
  char *ch = ctlr->parsePos;
  roadLive_t *live = &ctlr->roadLive;
  int nextStageLoaded = 0;

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
      ch = loadNextStage(ctlr);
      nextStageLoaded = 1;
    }
  }

  ctlr->parsePos = ch;
  return nextStageLoaded;
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

int roadCtlScroll(roadCtlr_t *ctlr, short zAmt)
{
  int i;
  int nextStageLoaded = 0;
  roadLive_t *live = &ctlr->roadLive;
  fixed_t xScrollAmount = (getCurveAmount(live) * (fixed_t)zAmt) << 0;
//  fixed_t yScrollAmount = -(getHillAmount(live) * (fixed_t)zAmt) >> 3;
//  fixed_t yOffsetDest = -getHillAmount(live) << 8;

  live->texOff =(live->texOff + zAmt) & ((1<<ZMAPBITS)-1);
  live->z += zAmt;
  live->segPos += zAmt<<10; // below 8 shaves off so much that it never moves
  if (live->segPos >= ZMAPLEN<<16)
  {
    live->segPos -= ZMAPLEN<<16;
    nextStageLoaded = parseCommand(ctlr);
  }

  





  // TODO: We may want a CarCtlr if car logic starts dominating this module
  for (i=0; i < MAXCARS; i++)
  {
    if (ctlr->carActive[i])
    {
      car_t *car = ctlr->cars + i;
      car->roadObj.z -= zAmt;
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
      // Randomized scenery
      int sideSlot = rand() & 1? 0 : 2;  // TODO: occasional something on road? We only do sides rn
      int gfxIndex = rand() & 1? ctlr->scenIndex1[sideSlot] : ctlr->scenIndex2[sideSlot];

      if (gfxIndex >= 0) {
        if (sideSlot == 0)
          roadCtlAddCar(ctlr, 85536 + (rand() & 131071), ZFARPLANE-1, 0, CARTYPE_TREE, gfxIndex);
        else if (sideSlot == 2)
          roadCtlAddCar(ctlr, -85536 - (rand() & 131071), ZFARPLANE-1, 0, CARTYPE_TREE, gfxIndex);
      }
    }
  }
  


  // Scroll horizon based on speed and turn sharpness
  live->xHorizon += xScrollAmount;
/*
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
*/
  live->yOffsetHorizon = 0; // Glue the horizon to the top of the road for now

  return nextStageLoaded;
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
        roadCtlRemoveCar(ctlr, car);
      }
      else if (car->roadObj.z <= ZNEARPLANE) {  // Deal with any special objects
        // Set scenery. This has to be done before it gets clipped.
        if (car->roadObj.car->type == CARTYPE_DUMMY_SCEN)
        {
          if (car->roadObj.x == 0)
            ctlr->roadLive.roadTex = car->roadObj.img;
          else if (car->roadObj.x == -1)
            ctlr->roadLive.sideTex[0] = car->roadObj.img;
          else if (car->roadObj.x == 1)
            ctlr->roadLive.sideTex[1] = car->roadObj.img;
        } 
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
        int lane = rand() % 3;
        car = roadCtlAddCar(ctlr, laneXs[lane], z, speed, CARTYPE_COUPE, texNum);
        lane = (lane + 1) % 3;
        car = roadCtlAddCar(ctlr, laneXs[lane], z - 512, speed, CARTYPE_COUPE, texNum);
    }
    else {
        const int lane = pattern;      
        // Left lane gets a speed boost
        if (lane == 0)
          speed += ITOFIX(20);
        car = roadCtlAddCar(ctlr, laneXs[lane], ZFARPLANE-1, speed, CARTYPE_COUPE, texNum);
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



